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
#include <string>
#ifdef _WIN32
#include <Windows.h>
#include <Winldap.h>
#include <Winber.h>
#else
#define LDAP_DEPRECATED 1
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

// TODO: Should probably define LDAP_UNICODE before including Winldap.h instead
#ifdef _WIN32
#define MyLDAPControl LDAPControlW
#define my_ldap_create_page_control ldap_create_page_controlW
#define my_ldap_parse_result ldap_parse_resultW
#define my_ldap_parse_page_control ldap_parse_page_controlW
#define my_ldap_controls_free ldap_controls_freeW
#define my_ldap_control_free ldap_control_freeW
#else
#define MyLDAPControl LDAPControl
#define my_ldap_create_page_control ldap_create_page_control
#define my_ldap_parse_result ldap_parse_result
#define my_ldap_parse_page_control ldap_parse_page_control
#define my_ldap_controls_free ldap_controls_free
#define my_ldap_control_free ldap_control_free
#endif

int ldap_search_ext_s_utf8(
    LDAP *ld,
    std::string base,
    int scope,
    std::string filter,
    char *attrs[],
    int attrsonly,
    MyLDAPControl **serverctrls,
    MyLDAPControl **clientctrls,    
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
        serverctrls, clientctrls, nullptr,
        LDAP_NO_LIMIT, msg);
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
        serverctrls, clientctrls, nullptr,
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

    struct search_state {
        std::string base;
        std::string filter;
        int scope_val;
        char **attrs_val = nullptr;
        LDAPMessage* result = nullptr;
        LDAPMessage* current_entry = nullptr;
        berval *cookie = nullptr;
    } ss;

    /**
     * Configuration file variables
     */

    std::string ldap_base{};
    std::string ldap_scope{};
    std::string ldap_filter{};
    std::string ldap_attrs{};
    std::string ldap_UUID{};
    bool paged_search;
    int page_size;

    std::string type{};
    pair_map multi_queries{};
    
     /**
      * LDAP state variables
      */
    class connection {
        std::string ldap_uri{};
        std::string ldap_who{};
        std::string ldap_password{};
        bool ldap_follow_referrals;

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
        cleanup_search_state();
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

        std::string ldap_page_size = config.get("ldap-page-size", true);
        paged_search = false;
        if (!ldap_page_size.empty()) {
            try {
                page_size = std::stoi(ldap_page_size);
            } catch (const std::exception&) {
                page_size  = 0;
            }

            if (page_size > 0) {
                paged_search = true;
            }
            else {
                std::cerr << "Invalid ldap-page-size: " << ldap_page_size << std::endl;
                std::cerr << "Running without paging" << std::endl;
            }
        }

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

    bool re_search() {
        int err = 0;
        MyLDAPControl *serverctrls[2] = { nullptr, nullptr };
        MyLDAPControl **clientctrls = nullptr;
        
        if (paged_search) {
            err = my_ldap_create_page_control(conn.simplescim_ldap_ld,
                                           page_size,
                                           ss.cookie, 'T', &serverctrls[0]);
            
            if (err != LDAP_SUCCESS) {
                std::cerr << "error creating paging control for LDAP search" << std::endl;
                return false;
            }
        }
        
        if (ss.result != nullptr) {
            ldap_msgfree(ss.result);
            ss.result = nullptr;
        }

        /** Search */
        err = ldap_search_ext_s_utf8(conn.simplescim_ldap_ld, ss.base, ss.scope_val,
                                ss.filter,
                                ss.attrs_val,
                                0,
                                serverctrls,
                                clientctrls,
                                LDAP_NO_LIMIT, 
                                &ss.result);

        if (err != LDAP_SUCCESS) {
            std::cerr << "error creating ldap search: " << ldap_err2string(err) << std::endl;
            std::cerr << "\ttype: " << type << ", base: " << ss.base << ", filter: " << ss.filter << std::endl;
            ldap_print_error(err, "ldap_search_ext_s");
            return false;
        }

        if (paged_search) {
#ifdef _WIN32
            ULONG errcode;
            ULONG total_count;
#else
            int errcode;
            ber_int_t total_count;
#endif
            MyLDAPControl **returned_controls = nullptr;

            // Parse the results to retrieve the contols being returned.
            err = my_ldap_parse_result(conn.simplescim_ldap_ld, ss.result, &errcode, NULL, NULL, NULL, &returned_controls, 0);

            if (err != LDAP_SUCCESS) {
                std::cerr << "error retrieving LDAP server controls for paging" << std::endl;
                return false;
            }

            if (ss.cookie != nullptr) {
                ber_bvfree(ss.cookie);
                ss.cookie = nullptr;
            }

            // Parse the page control returned to get the cookie
            err = my_ldap_parse_page_control(conn.simplescim_ldap_ld, returned_controls, &total_count, &ss.cookie);

            if (err != LDAP_SUCCESS) {
                std::cerr << "failed to get LDAP paging cookie" << std::endl;
                return false;
            }

            /* Cleanup the controls used. */
            if (returned_controls != nullptr) {
                my_ldap_controls_free(returned_controls);
            }
            my_ldap_control_free(serverctrls[0]);
        }

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
        
        if (scopes.find(ldap_scope) != scopes.end()) {
            ss.scope_val = scopes[ldap_scope];
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

        ss.base = filter_val.first;
        ss.filter = filter_val.second;

        /** Parse attrs */
        int err = simplescim_ldap_attrs_parser(ldap_attrs.c_str(), &ss.attrs_val);

        if (err == -1) {
            return false;
        }

        if (ss.cookie != nullptr) {
            ber_bvfree(ss.cookie);
            ss.cookie = nullptr;
        }

        load_logger.log("Searching for " + type +
                        ", base: " + ss.base +
                        ", filter: " + ss.filter);

        if (!re_search()) {
            throw std::string("exiting");
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

                if (err == LDAP_OPT_SUCCESS && ld_errno == LDAP_OPT_SUCCESS) {
                    // The attribute simply had no values, skip it
                    ldap_memfree(attr);
                    continue;
                }

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
                    std::string st;
                    if (config_file::instance().get_bool("ldap-MS-UUID")) {
                        st = uuid_util::instance().un_parse_ms_uuid(vals[i]->bv_val);
                    }
                    else {
                        st = uuid_util::instance().un_parse_uuid(vals[i]->bv_val);
                    }
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
        ss.current_entry = ldap_first_entry(conn.simplescim_ldap_ld, ss.result);

        if (ss.current_entry == nullptr) {
            cleanup_search_state();
            return nullptr;
        }

        return entry_to_base_object(ss.current_entry);
    }
  
    std::shared_ptr<base_object> next_object() {
        ss.current_entry = ldap_next_entry(conn.simplescim_ldap_ld, ss.current_entry);

        if (ss.current_entry == nullptr) {
            if (paged_search && ss.cookie != nullptr && ss.cookie->bv_val != nullptr && (strlen(ss.cookie->bv_val) > 0)) {
                if (!re_search()) {
                    std::cerr << "failed when getting next page" << std::endl;
                    cleanup_search_state();
                    return nullptr;
                }
                return first_object();
            }
            else {
                cleanup_search_state();
                return nullptr;
            }
        }

        return entry_to_base_object(ss.current_entry);
    }

    void cleanup_search_state() {
        if (ss.result != nullptr) {
            ldap_msgfree(ss.result);
            ss.result = nullptr;
        }

        if (ss.attrs_val != nullptr) {
            for (size_t i = 0; ss.attrs_val[i] != nullptr; ++i) {
                free(ss.attrs_val[i]);
            }
            free(ss.attrs_val);
            ss.attrs_val = nullptr;
        }

        if (ss.cookie != nullptr) {
            ber_bvfree(ss.cookie);
            ss.cookie = nullptr;
        }
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

    /* Set whether or not to follow referrals */

    err = ldap_set_option(simplescim_ldap_ld, LDAP_OPT_REFERRALS, ldap_follow_referrals ? LDAP_OPT_ON : LDAP_OPT_OFF);

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

    const auto follow_referrals_arg = "ldap-follow-referrals";
    if (con.has(follow_referrals_arg)) {
        ldap_follow_referrals = con.get_bool(follow_referrals_arg);
    }
    else {
        ldap_follow_referrals = true;
    }
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

