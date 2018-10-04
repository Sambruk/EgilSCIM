//
// Created by Ola Mattsson on 2018-09-22.
//

#include <set>
#include "ldap_wrapper.hpp"
#include "simplescim_ldap.hpp"
#include "uuid/uuid.h"

ldap_wrapper::ldap_wrapper() {
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

bool ldap_wrapper::ldap_get_variables() {

	ldap_base = config.get("ldap-base");
	ldap_scope = config.get("ldap-scope");
	ldap_filter = config.get("ldap-filter", true);
	ldap_attrs = config.get("ldap-attrs", true);
	ldap_attrsonly = config.get("ldap-attrsonly");
	if (!ldap_attrs.empty())
		std::cout
				<< "ldap_attrs are generated from variables used in the scim templates. It is safe to leave this empty"
				<< std::endl;
	return true;
}

bool ldap_wrapper::ldap_get_type_variables() {
//	std::string type_filters = config_file::instance().get(type + "-ldap-filter", true);
//	if (type_filters.find("queries") != std::string::npos) {
	multi_queries = json_data_file::json_to_ldap_query(type);
//	}
	return true;
}


bool ldap_wrapper::search(const std::string &intype, const std::pair<std::string, std::string> &filters) {
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
		return false;
	}

	return true;
}

//#include <boost/uuid/uuid.hpp>
//#include <boost/uuid/uuid_generators.hpp>
//#include <boost/uuid/uuid_io.hpp>
//#include <boost/lexical_cast.hpp>

std::shared_ptr<base_object> ldap_wrapper::entry_to_user(LDAPMessage *entry) {
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
			if (attr_clone == "GUID") {
				char ascii[100];
//				char *tmp = vals[i]->bv_val;
//				std::string stmp = tmp;
//				boost::uuids::uuid u = boost::lexical_cast<boost::uuids::uuid>(stmp);
				uuid_unparse((unsigned char *) vals[i]->bv_val, ascii);
				list.emplace_back(ascii);
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

std::shared_ptr<object_list> ldap_wrapper::ldap_to_user_list() {
	std::shared_ptr<object_list> users;
	std::string uid;
	LDAPMessage *entry;

	/** Initialise user list */
	users = std::make_shared<object_list>();
	connection &con = connection::instance();
	for (entry = ldap_first_entry(con.simplescim_ldap_ld, simplescim_ldap_res);
	     entry != nullptr; entry = ldap_next_entry(con.simplescim_ldap_ld, entry)) {

		/* Create the user. */

		std::shared_ptr<base_object> user = entry_to_user(entry);

		if (user == nullptr) {
			return users;
		}

		/** Get the user's unique identifier. */

		uid = user->get_uid();

		if (uid.empty()) {
			continue;
		}

		/** Insert user into user list. */
		users->add_object(uid, user);
		load_related(user->getSS12000type(), users);
	}
	return users;
}

bool ldap_wrapper::connection::ldap_init() {
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

int ldap_wrapper::connection::get_variables() {
	config_file &con = config_file::instance();
	ldap_uri = con.get("ldap-uri");
	ldap_who = con.get("ldap-who");
	ldap_password = con.get("ldap-passwd");
	return 0;
}

void ldap_wrapper::connection::ldap_close() {

	if (simplescim_ldap_ld != nullptr) {
		/* Disregard the return value. */
		ldap_unbind_ext(simplescim_ldap_ld, nullptr, nullptr);
		simplescim_ldap_ld = nullptr;
	}
}

