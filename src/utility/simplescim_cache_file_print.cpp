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
 *
 * Further development with groups and relations support
 * by Ola Mattsson - IT informa for Sambruk
 */

#include <stdio.h>
#include <stdlib.h>

#include "simplescim_error_string.hpp"
#include "../model/object_list.hpp"
#include "../cache_file.hpp"
#include "../config_file.hpp"
static int print_attr(const std::string &attribute, const string_vector &values) {

	for (const auto &value : values) {
		std::cout << attribute << ": " << value << std::endl;
	}

	return 0;
}

static int print_user(const std::string &uid, const base_object &user) {
	std::cout << "==== " << user.getSS12000type() << " ====" << std::endl
	          << "== Unique identifier: " << uid << std::endl;
	std::for_each(user.begin(), user.end(), [](const auto &item) {
		print_attr(item.first, item.second);
	});

	return 0;
}

int main(int argc, char *argv[]) {
	config_file &config = config_file::instance();
	if (argc != 2) {
		fprintf(stderr, "Usage: %s cache-file\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	int err = config.load(argv[1]);

	auto users = cache_file::instance().get_contents();

	if (users == nullptr) {
		fprintf(stderr, "%s\n", simplescim_error_string_get());
		exit(EXIT_FAILURE);
	}
	std::for_each(users->begin(), users->end(), [](const auto &item) {
		print_user(item.first, *item.second);
	});

	return 0;
}
