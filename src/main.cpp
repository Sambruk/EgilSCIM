/**
 * Copyright © 2017-2018  Max Wällstedt <>
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

//#include <stdio.h>
#include <iostream>

#include "utility/simplescim_error_string.hpp"
#include "model/object_list.hpp"
#include "config_file.hpp"
#include "simplescim_ldap.hpp"
#include "cache_file.hpp"
#include "scim.hpp"
#include "scim_data.hpp"
#include "json_data_file.hpp"
#include "utility/utils.hpp"
#include "data_server.hpp"


int main(int argc, char *argv[]) {
	config_file &config = config_file::instance();

    int param_start = check_params(argc, argv);
    std::string test_url;
	if (param_start < 0)
		return 1;
	else if (param_start == 1)
	    test_url = get_test_server_url(argv);





	for (int i = 1+param_start; i < argc; ++i) {
		/** Load configuration file */
		std::cout << "processing: " << argv[i] << std::endl;
        int err = 0;
        try {
            err = config.load(argv[i]);
        } catch (std::string& msg) {
		    std::cerr << msg << std::endl;
		    exit(1);
		}

		if (err == -1) {
			print_error();
			continue;
		}
		if (!test_url.empty())
			config.replace_variable("scim-url", test_url);

		/** Get objects from LDAP catalogue */
		data_server &server = data_server::instance();
		try {
            server.load();
        } catch (std::string& msg) {
		    std::cerr << msg << std::endl;
		    exit(1);
		}
		if (server.empty()) {
			print_error();
			config.clear();
			server.clear();
			continue;
		}


		/** Get objects from cache file */
		std::shared_ptr<object_list> cache = cache_file::instance().get_contents();

		if (cache == nullptr) {
			print_error();
			server.clear();
			config.clear();
			continue;
		}

		/** Perform SCIM operations */
		try {
			err = ScimActions().perform(server, *cache);
		} catch (const std::string& err_msg) {
			std::cerr << err_msg << std::endl;
		}

		if (err == -1) {
			server.clear();
			print_error();
			config.clear();
			continue;
		}

		print_status(argv[i]);
        print_error();

		/* Clean up */
		config.clear();
		server.clear();
	}

	return 0;
}
