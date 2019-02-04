/**
 * Created by Ola Mattsson.
 *
 * This file is part of EgilSCIM.
 *
 * EgilSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EgilSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with EgilSCIM.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Further development with groups and relations support
 * by Ola Mattsson - IT informa for Sambruk
 */

#include "ldap_wrapper.hpp"
#include <set>
#include <ldap.h>
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

  
/**
 * LDAP state variables
 */
class connection {
  connection() = default;

public:
  std::string ldap_uri{};
  std::string ldap_who{};
  std::string ldap_password{};
  LDAP *simplescim_ldap_ld = nullptr;

  int get_variables();

  static connection &instance() {
    static connection con;
    return con;
  }

  /**
   * Initialises LDAP session.
   */
  bool ldap_init();

  void ldap_close();

  bool initialised = false;
};

} // namespace

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
  std::pair<std::string, std::string> override_filter{};
  pair_map multi_queries{};

  Impl() {
    if (!connection::instance().initialised) {
      connection::instance().ldap_init();
    }
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
    if (ldap_UUID.empty()) {
      ldap_UUID = "GUID";
      std::cout << "ldap variable ldap_UUID is missing. GUID assumed. If your catalogue \n"
	"is using a different attibute name for unique id enter this in the configuration. \n"
	"for example, openldap uses entityUUID. \n"
	"If your unique id is indeed GUID, enter this variable anyway to silence this message." << std::endl;
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

  bool search(const std::string &intype, const std::pair<std::string, std::string> &filters) {
    if (!connection::instance().initialised)
      return false;

    if (!intype.empty())
      type = intype;

    if (!ldap_get_type_variables())
      return false;

    if (!filters.first.empty() && !filters.second.empty())
      override_filter = filters;

    /** Set search scope */
    int scope_val;
    if (ldap_scope == "BASE") {
      scope_val = LDAP_SCOPE_BASE;
    } else if (ldap_scope == "ONELEVEL") {
      scope_val = LDAP_SCOPE_ONELEVEL;
    } else if (ldap_scope == "SUBTREE") {
      scope_val = LDAP_SCOPE_SUBTREE;
    } else if (ldap_scope == "CHILDREN") {
      scope_val = LDAP_SCOPE_CHILDREN;
    } else {
      simplescim_error_string_set_prefix("simplescim_ldap_search");
      simplescim_error_string_set_message("variable \"ldap-scope\" has invalid value \"%s\"\n"
					  "variable \"ldap-scope\" must have one of the following values:\n"
					  " BASE ONELEVEL SUBTREE CHILDREN", ldap_scope.c_str());
      return false;
    }


    multi_queries.find(type);
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

    /** Search */
    err = ldap_search_ext_s(connection::instance().simplescim_ldap_ld, filter_val.first.c_str(), scope_val,
			    filter_val.second.c_str(),
			    attrs_val,
			    attrsonly_val,
			    nullptr, nullptr, nullptr, -1, &simplescim_ldap_res);

    /** Free attrs_val if it is not nullptr. */
    if (attrs_val != nullptr) {
      for (size_t i = 0; attrs_val[i] != nullptr; ++i) {
	free(attrs_val[i]);
      }
      free(attrs_val);
    }

    /** Check if the search operation returned an error. */
    if (err != LDAP_SUCCESS) {
      std::cout << "error creating ldap search: " << ldap_err2string(err) << std::endl;
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
    for (char *attr = ldap_first_attribute(connection::instance().simplescim_ldap_ld, entry, &ber);
	 attr != nullptr; attr = ldap_next_attribute(connection::instance().simplescim_ldap_ld, entry, ber)) {

      /** Get and clone 'vals'. */
      berval **vals = ldap_get_values_len(connection::instance().simplescim_ldap_ld, entry, attr);

      if (vals == nullptr) {
	int ld_errno;
	int err = ldap_get_option(connection::instance().simplescim_ldap_ld, LDAP_OPT_RESULT_CODE, &ld_errno);

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
    connection &con = connection::instance();
    current_entry = ldap_first_entry(con.simplescim_ldap_ld, simplescim_ldap_res);

    if (current_entry == nullptr) {
      return nullptr;
    }

    return entry_to_base_object(current_entry);
  }
  
  std::shared_ptr<base_object> next_object() {
    connection &con = connection::instance();
    current_entry = ldap_next_entry(con.simplescim_ldap_ld, current_entry);

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

void ldap_wrapper::ldap_close() {
  connection::instance().ldap_close();
}

bool ldap_wrapper::valid() {
  return connection::instance().initialised;
}

bool ldap_wrapper::search(const std::string &intype, const std::pair<std::string, std::string> &filters) {
  return impl->search(intype, filters);
}

std::shared_ptr<base_object> ldap_wrapper::first_object() {
  return impl->first_object();
}

std::shared_ptr<base_object> ldap_wrapper::next_object() {
  return impl->next_object();
}

bool connection::ldap_init() {
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

  cred.bv_val = (char *) ldap_password.c_str();
  cred.bv_len = ldap_password.length();

  err = ldap_sasl_bind_s(simplescim_ldap_ld, ldap_who.c_str(), LDAP_SASL_SIMPLE, &cred, nullptr, nullptr,
			 nullptr);

  if (err != LDAP_SUCCESS) {
    ldap_print_error(err, "ldap_sasl_bind_s");
    ldap_close();
    return false;
  }
  initialised = true;
  return true;
}

int connection::get_variables() {
  config_file &con = config_file::instance();
  ldap_uri = con.get("ldap-uri");
  ldap_who = con.get("ldap-who");
  ldap_password = con.get("ldap-passwd");
  return 0;
}

void connection::ldap_close() {

  if (simplescim_ldap_ld != nullptr) {
    /* Disregard the return value. */
    ldap_unbind_ext(simplescim_ldap_ld, nullptr, nullptr);
    simplescim_ldap_ld = nullptr;
  }
}

