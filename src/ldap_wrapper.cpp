/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2019 Föreningen Sambruk
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.

 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "ldap_wrapper.hpp"
#include <set>
#ifdef _WIN32
#include <Windows.h>
#include <Winldap.h>
#include <Winber.h>
#else
#include <ldap.h>
#endif
#include "json_data_file.hpp"
#include "config_file.hpp"
#include "utility/simplescim_error_string.hpp"
#include "simplescim_ldap_attrs_parser.hpp"

namespace {
/**
 * Prints an error message concerning LDAP to
 * simplescim_error_string.
 */
void ldap_print_error(int err, const char *func) {
    simplescim_error_string_set_prefix("%s", func);
    simplescim_error_string_set_message("%s", ldap_err2string(err));
}

int ldap_search_ext_s_utf8(
    LDAP *ld,
    std::string base,
    int scope,
    std::string filter,
    char *attrs[],
    int attrsonly,
    int sizeLimit,
    LDAPMessage **msg
) {
#ifdef _WIN32
    const auto buffer_size = 1024;
    WCHAR base_buffer[buffer_size];
    WCHAR filter_buffer[buffer_size];
    int n = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, base.c_str(), (int)base.size(), base_buffer, buffer_size);
    base_buffer[n] = 0;
    n = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, filter.c_str(), (int)filter.size(), filter_buffer, buffer_size);
    filter_buffer[n] = 0;

    // Convert attrs to WCHAR**
    int num_attrs = 0;
    while (attrs != nullptr && attrs[num_attrs] != nullptr) {
        ++num_attrs;
    }

    std::vector<WCHAR*> wattrs;
    std::vector<std::vector<WCHAR>> storage;

    for (int i = 0; i < num_attrs; ++i) {
        storage.emplace_back(buffer_size, 0);
        wattrs.push_back(&(storage.back()[0]));
        int n = MultiByteToWideChar(CP_UTF8, MB_PRECOMPOSED, attrs[i], (int)strlen(attrs[i]), wattrs[i], buffer_size);
        wattrs[i][n] = 0;
    }
    wattrs[num_attrs] = nullptr;

    return ldap_search_ext_sW(ld, base_buffer, scope,
        filter_buffer,
        &wattrs[0],
        attrsonly,
        nullptr, nullptr, nullptr, LDAP_NO_LIMIT, msg);
#else
    // For some reason, ldap_search_ext_s expects char *, not const char *,
    // so we'll copy...
    std::vector<char> base_copy(base.begin(), base.end());
    base_copy.push_back(0);
    std::vector<char> filter_copy(filter.begin(), filter.end());
    filter_copy.push_back(0);

    return ldap_search_ext_s(ld, &base_copy[0], scope,
        &filter_copy[0],
        attrs,
        attrsonly,
        nullptr, nullptr, nullptr,
        sizeLimit,
        msg);
#endif
}

#ifdef _WIN32
int ldap_initialize(LDAP** ldp, const char* uri) {

    bool ssl = startsWith(uri, "ldaps://");

    std::string host = uri;

    auto pos = host.find("://");
    if (pos != std::string::npos) {
        host = host.substr(pos + 3);
    }

    char* host_copy = _strdup(host.c_str());
    if (ssl) {
        *ldp = ldap_sslinit(host_copy, LDAP_SSL_PORT, 1);
    }
    else {
        *ldp = ldap_init(host_copy, LDAP_PORT);
    }
    free(host_copy);

    if (*ldp == NULL) {
        return LdapGetLastError();
    }
    else {
        return LDAP_SUCCESS;
    }
}
#endif
} // namespace

#ifndef LDAP_OPT_RESULT_CODE // doesn't seem to be defined on win32, use the old name
#define LDAP_OPT_RESULT_CODE LDAP_OPT_ERROR_NUMBER
#endif

#ifndef LDAP_OPT_SUCCESS // win32 doesn't have a specific success return code for ldap_get_option
#define LDAP_OPT_SUCCESS LDAP_SUCCESS
#endif

struct ldap_wrapper::Impl {
    const config_file &config = config_file::instance();

    LDAPMessage* simplescim_ldap_res = nullptr;
    LDAPMessage* current_entry = nullptr;

    /**
     * Configuration file variables
     */

    std::string ldap_base{};
    std::string ldap_scope{};
    std::string ldap_filter{};
    std::string ldap_attrs{};
    std::string ldap_UUID{};
    std::string ldap_attrsonly{};

    std::string type{};
    pair_map multi_queries{};
    
     /**
      * LDAP state variables
      */
    class connection {
        std::string ldap_uri{};
        std::string ldap_who{};
        std::string ldap_password{};

        /**
         * Initialises LDAP session.
         */
        bool ldap_init();
        
        void ldap_close();

        int get_variables();        
        
    public:

        connection() {
            ldap_init();
        }
        
        ~connection() {
            ldap_close();
        }
        
        LDAP *simplescim_ldap_ld = nullptr;
                        
        bool initialised = false;
    };

    connection conn;

    Impl() {
        ldap_get_variables();

        std::vector<std::string> attributes = config_file::instance().get_vector("all-scim-variables");
        std::set<std::string> unique_strings;

        for (auto &&a: attributes) {
            if (a.find('.') != std::string::npos)
                unique_strings.emplace(string_to_pair(a).second);
            else
                unique_strings.emplace(a);
        }

        for (auto &&a: unique_strings) {
            ldap_attrs += a + ",";
        }

        if (*ldap_attrs.rbegin() == ',')
            ldap_attrs.erase(ldap_attrs.length() - 1, 1);

    }

    ~Impl() {
        if (simplescim_ldap_res != nullptr) {
            /* Disregard the return value. */
            ldap_msgfree(simplescim_ldap_res);
            simplescim_ldap_res = nullptr;
        }
    }
  
    /**
     * Gets and verifies all LDAP variables from the configuration file.
     */
    bool ldap_get_variables() {

        ldap_base = config.get("ldap-base");
        ldap_scope = config.get("ldap-scope");
        ldap_filter = config.get("ldap-filter", true);
        ldap_attrs = config.get("ldap-attrs", true);
        ldap_UUID = config.get("ldap-UUID", true);
        ldap_attrsonly = config.get("ldap-attrsonly");

        if (!ldap_attrs.empty())
            std::cout
                << "ldap_attrs are generated from variables used in the scim templates. It is safe to leave this empty"
                << std::endl;
        return true;
    }

    bool ldap_get_type_variables() {
        //	std::string type_filters = config_file::instance().get(type + "-ldap-filter", true);
        //	if (type_filters.find("queries") != std::string::npos) {
        multi_queries = json_data_file::json_to_ldap_query(type);
        //	}
        return true;
    }

    bool search(const std::string &intype,
                indented_logger& load_logger,
                const std::pair<std::string, std::string> &filters) {
        if (!conn.initialised)
            return false;

        if (!intype.empty())
            type = intype;

        if (!ldap_get_type_variables())
            return false;

        std::pair<std::string, std::string> override_filter{};        
        if (!filters.first.empty() && !filters.second.empty())
            override_filter = filters;

        /** Set search scope */
        auto scopes = std::map<std::string, int>{
            {"BASE", LDAP_SCOPE_BASE},
            {"ONELEVEL", LDAP_SCOPE_ONELEVEL},
            {"SUBTREE", LDAP_SCOPE_SUBTREE},
#ifndef _WIN32
            {"CHILDREN", LDAP_SCOPE_CHILDREN},
#endif
        };
        int scope_val;
        if (scopes.find(ldap_scope) != scopes.end()) {
            scope_val = scopes[ldap_scope];
        }
        else {
            std::string possible_scopes;

            for (auto pair : scopes) {
                possible_scopes += pair.first + " ";
            }

            simplescim_error_string_set_prefix("simplescim_ldap_search");
            simplescim_error_string_set_message("variable \"ldap-scope\" has invalid value \"%s\"\n"
                                                "variable \"ldap-scope\" must have one of the following values:\n"
                                                " %s", ldap_scope.c_str(), possible_scopes.c_str());
            return false;
        }


        /** Set filter */
        std::pair<std::string, std::string> filter_val(ldap_base, "");
        if (!override_filter.second.empty()) {
            filter_val = override_filter;
        } else if (!multi_queries.empty()) {
            auto f = multi_queries.find(type);
            if (f != multi_queries.end()) {
                filter_val = f->second;
            }

        }

        /** Parse attrs */
        char **attrs_val;
        int err = simplescim_ldap_attrs_parser(ldap_attrs.c_str(), &attrs_val);

        if (err == -1) {
            return false;
        }

        /** Set attrsonly */
        int attrsonly_val;
        if (ldap_attrsonly == "TRUE") {
            attrsonly_val = 1;
        } else if (ldap_attrsonly == "FALSE") {
            attrsonly_val = 0;
        } else {
            simplescim_error_string_set_prefix("simplescim_ldap_search");
            simplescim_error_string_set_message("variable \"ldap-attrsonly\" has invalid value \"%s\"\n"
                                                "variable \"ldap-attrsonly\" must have one of the following values:\n"
                                                " TRUE FALSE", ldap_attrsonly.c_str());
            return false;
        }

        load_logger.log("Searching for " + type +
                        ", base: " + filter_val.first +
                        ", filter: " + filter_val.second);
        
        /** Search */
        err = ldap_search_ext_s_utf8(conn.simplescim_ldap_ld, filter_val.first, scope_val,
                                filter_val.second,
                                attrs_val,
                                attrsonly_val,
                                LDAP_NO_LIMIT, 
                                &simplescim_ldap_res);

        /** Free attrs_val if it is not nullptr. */
        if (attrs_val != nullptr) {
            for (size_t i = 0; attrs_val[i] != nullptr; ++i) {
                free(attrs_val[i]);
            }
            free(attrs_val);
        }

        /** Check if the search operation returned an error. */
        if (err != LDAP_SUCCESS) {
            std::cerr << "error creating ldap search: " << ldap_err2string(err) << std::endl;
            std::cerr << "\ttype: " << intype << ", base: " << filter_val.first.c_str() << ", filter: " << filter_val.second.c_str() << std::endl;
            ldap_print_error(err, "ldap_search_ext_s");
            throw std::string("exiting");
            //		return false;
        }

        return true;
    }

    /**
     * Converts an entry in the LDAP search results into a base_object.
     * On success, a pointer to a new user object is returned.
     * On error, nullptr is returned and simplescim_error_string
     * is set to an appropriate error message.
     */
    std::shared_ptr<base_object> entry_to_base_object(LDAPMessage *entry) {
        BerElement *ber;

        attrib_map attributes;

        /** Create the user object. */
        for (char *attr = ldap_first_attribute(conn.simplescim_ldap_ld, entry, &ber);
             attr != nullptr; attr = ldap_next_attribute(conn.simplescim_ldap_ld, entry, ber)) {

            /** Get and clone 'vals'. */
            berval **vals = ldap_get_values_len(conn.simplescim_ldap_ld, entry, attr);

            if (vals == nullptr) {
                int ld_errno;
                int err = ldap_get_option(conn.simplescim_ldap_ld, LDAP_OPT_RESULT_CODE, &ld_errno);

                if (err != LDAP_OPT_SUCCESS) {
                    ldap_print_error(err, "ldap_get_option");
                } else {
                    ldap_print_error(ld_errno, "ldap_get_values_len");
                }

                ldap_memfree(attr);
                ber_free(ber, 0);
                return nullptr;
            }

            size_t len = 0;

            for (size_t i = 0; vals[i] != nullptr; ++i) {
                ++len;
            }

            /** Clone and insert all values */
            string_vector list;
            std::string attr_clone = attr;
            for (size_t i = 0; i < len; ++i) {
                if (attr_clone == ldap_UUID) {
                    std::string st = uuid_util::instance().un_parse_uuid(vals[i]->bv_val);
                    list.emplace_back(st);
                } else {
                    list.emplace_back(std::string(vals[i]->bv_val));
                }
            }
            attributes.emplace(attr_clone, list);

            /** Free LDAP data */
            ldap_value_free_len(vals);
            ldap_memfree(attr);
        }
        attributes.emplace(std::make_pair("ss12000type", string_vector({type})));
        std::shared_ptr<base_object> user = std::make_shared<base_object>(std::move(attributes));

        ber_free(ber, 0);

        return user;
    }

    std::shared_ptr<base_object> first_object() {
        current_entry = ldap_first_entry(conn.simplescim_ldap_ld, simplescim_ldap_res);

        if (current_entry == nullptr) {
            return nullptr;
        }

        return entry_to_base_object(current_entry);
    }
  
    std::shared_ptr<base_object> next_object() {
        current_entry = ldap_next_entry(conn.simplescim_ldap_ld, current_entry);

        if (current_entry == nullptr) {
            return nullptr;
        }

        return entry_to_base_object(current_entry);
    }
};

ldap_wrapper::ldap_wrapper()
        : impl(new Impl()) {
}

ldap_wrapper::~ldap_wrapper() {
}  

bool ldap_wrapper::valid() {
    return impl->conn.initialised;
}

bool ldap_wrapper::search(const std::string &intype,
                          indented_logger& load_logger,
                          const std::pair<std::string, std::string> &filters) {
    return impl->search(intype, load_logger, filters);
}

std::shared_ptr<base_object> ldap_wrapper::first_object() {
    return impl->first_object();
}

std::shared_ptr<base_object> ldap_wrapper::next_object() {
    return impl->next_object();
}

bool ldap_wrapper::Impl::connection::ldap_init() {
    int ldap_version = LDAP_VERSION3;
    struct berval cred{};
    int err;

    /* Get configuration file variables related to LDAP */

    err = get_variables();

    if (err == -1) {
        return false;
    }

    /* Initialise LDAP session */

    err = ldap_initialize(&simplescim_ldap_ld, ldap_uri.c_str());

    if (err != LDAP_SUCCESS) {
        ldap_print_error(err, "ldap_initialize");
        return false;
    }

    /* Set protocol version */

    err = ldap_set_option(simplescim_ldap_ld, LDAP_OPT_PROTOCOL_VERSION, &ldap_version);

    if (err != LDAP_OPT_SUCCESS) {
        ldap_print_error(err, "ldap_set_option");
        ldap_close();
        return false;
    }

    /* Perform bind */

#ifndef _WIN32
    cred.bv_val = (char *) ldap_password.c_str();
    cred.bv_len = ldap_password.length();

    err = ldap_sasl_bind_s(simplescim_ldap_ld, ldap_who.c_str(), LDAP_SASL_SIMPLE, &cred, nullptr, nullptr,
                           nullptr);
#else
    char* who = _strdup(ldap_who.c_str());
    char* password = _strdup(ldap_password.c_str());
    err = ldap_simple_bind_s(simplescim_ldap_ld, who, password);
    free(who);
    free(password);
#endif

    if (err != LDAP_SUCCESS) {
        ldap_print_error(err, "ldap_sasl_bind_s");
        ldap_close();
        return false;
    }
    initialised = true;
    return true;
}

int ldap_wrapper::Impl::connection::get_variables() {
    config_file &con = config_file::instance();
    ldap_uri = con.get("ldap-uri");
    ldap_who = con.get("ldap-who");
    ldap_password = con.get("ldap-passwd");
    return 0;
}

void ldap_wrapper::Impl::connection::ldap_close() {

    if (simplescim_ldap_ld != nullptr) {
        /* Disregard the return value. */
#ifndef _WIN32
        ldap_unbind_ext(simplescim_ldap_ld, nullptr, nullptr);
#else
        ldap_unbind_s(simplescim_ldap_ld);
#endif
        simplescim_ldap_ld = nullptr;
    }
}

