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

#ifndef EGILSCIMCLIENT_LDAP_WRAPPER_HPP
#define EGILSCIMCLIENT_LDAP_WRAPPER_HPP

#include <ldap.h>
#include "config_file.hpp"
#include "json_data_file.hpp"
#include "utility/simplescim_error_string.hpp"
#include "simplescim_ldap_attrs_parser.hpp"

class ldap_wrapper {
	const config_file &config = config_file::instance();

	LDAPMessage *simplescim_ldap_res = nullptr;

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

public:

	explicit ldap_wrapper();

	~ldap_wrapper() {
		if (simplescim_ldap_res != nullptr) {
			/* Disregard the return value. */
			ldap_msgfree(simplescim_ldap_res);
			simplescim_ldap_res = nullptr;
		}
	}

	void ldap_close();

	bool valid();

	/**
	 * Gets and verifies all LDAP variables from the configuration file.
	 */
	bool ldap_get_variables();

	bool ldap_get_type_variables();


	/**
	 * Performs the LDAP search operation.
	 */
	bool search(const std::string &type, const std::pair<std::string, std::string> &filters = {"", ""});

	/**
	 * Converts an entry in the LDAP search results into a user.
	 * On success, a pointer to a new user object is returned.
	 * On error, nullptr is returned and simplescim_error_string
	 * is set to an appropriate error message.
	 */
	std::shared_ptr<base_object> entry_to_user(LDAPMessage *entry);

	/**
	 * Construct the user list object from the LDAP response.
	 * On success, a pointer to the constructed object is
	 * returned. On error, nullptr is returned and
	 * simplescim_error_string is set to an appropriate
	 * error message.
	 */
	std::shared_ptr<object_list> ldap_to_user_list();
};


#endif //EGILSCIMCLIENT_LDAP_WRAPPER_HPP
