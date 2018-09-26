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

#ifndef SIMPLESCIM_CACHE_FILE_H
#define SIMPLESCIM_CACHE_FILE_H

#include <functional>
#include <cstdint>
#include <cstddef>
#include <string>
#include "model/base_object.hpp"

class object_list;

class base_object;


/**
 * Reads cache file specified in configuration file and
 * constructs a user list according to its contents.
 * On success, a pointer to the constructed user list is
 * returned. If the cache file doesn't exist, an empty user
 * list is returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
class cache_file {
	int cache_file_fd{};
	std::string cache_file_filename;
public:
	static cache_file &instance() {
		static cache_file the_cache;
		return the_cache;
	}

	~cache_file();

	std::shared_ptr<object_list> get_users();

	std::shared_ptr<object_list>
	get_users_from_file(const char *filename);

	int save(const object_list *objects);

private:
	int write_n(const uint8_t *buf, size_t n);

	int write_uint64(uint64_t n);

	int write_value(const std::string &av);

	int write_value_list(const std::vector<std::string> &al);

	int write_attribute(const std::string &attribute, const std::vector<std::string> &values);

	int write_user(const std::string &uid, const base_object &user);

	int read_n(uint8_t *buf, size_t n);

	int read_uint64(uint64_t *buf);

	int read_value(std::string *avp);

	int read_value_list(std::vector<std::string> *alp);

	int read_users(std::shared_ptr<object_list> usersp);

	std::shared_ptr<base_object> read_user(std::string *uidp);


};

#endif
