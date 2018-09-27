/**
 * Copyright © 2017-2018  Max Wällstedt <>
 *
 * This file is part of SimpleSCIM.
 *
 * SimpleSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with SimpleSCIM.  If not, see <http://www.gnu.org/licenses/>.
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

	if (check_params(argc, argv) != 0)
		return 1;


	for (int i = 1; i < argc; ++i) {
		/** Load configuration file */
		std::cout << "processing: " << argv[i] << std::endl;
		int err = config.load(argv[i]);

		if (err == -1) {
			print_error();
			continue;
		}

		/** Get objects from LDAP catalogue */
		data_server &server = data_server::instance();
		server.load();

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
		err = ScimActions().perform(server, *cache);

		if (err == -1) {
			server.clear();
			print_error();
			config.clear();
			continue;
		}

		print_status(argv[i]);

		/* Clean up */
		config.clear();
		server.clear();
	}

	return 0;
}
