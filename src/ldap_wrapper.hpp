//
// Created by Ola Mattsson on 2018-09-22.
//

#ifndef EGILSCIMCLIENT_LDAP_WRAPPER_HPP
#define EGILSCIMCLIENT_LDAP_WRAPPER_HPP

#include <ldap.h>
//#include <uuid/uuid.h>
#include "config_file.hpp"
#include "json_data_file.hpp"
#include "utility/simplescim_error_string.hpp"
#include "simplescim_ldap_attrs_parser.hpp"

class ldap_wrapper {
	const config_file &config = config_file::instance();

	LDAPMessage *simplescim_ldap_res = nullptr;

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

	/**
	 * Configuration file variables
	 */

	std::string ldap_base{};
	std::string ldap_scope{};
	std::string ldap_filter{};
	std::string ldap_attrs{};
	std::string ldap_attrsonly{};

	std::string type{};
	std::pair<std::string, std::string> override_filter{};
	pair_map multi_queries{};

	/**
	 * Prints an error message concerning LDAP to
	 * simplescim_error_string.
	 */
	static void ldap_print_error(int err, const char *func) {
		simplescim_error_string_set_prefix("%s", func);
		simplescim_error_string_set_message("%s", ldap_err2string(err));
	}

public:

	explicit ldap_wrapper();

	~ldap_wrapper() {
		if (simplescim_ldap_res != nullptr) {
			/* Disregard the return value. */
			ldap_msgfree(simplescim_ldap_res);
			simplescim_ldap_res = nullptr;
		}
	}

	/**
	 * Terminates an LDAP session and frees any dynamically
	 * allocated memory associated with it.
	 */
	void ldap_close() {
		connection::instance().ldap_close();

	}

	bool valid() {
		return connection::instance().initialised;
	}

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
