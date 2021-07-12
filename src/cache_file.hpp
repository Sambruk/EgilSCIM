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

#ifndef EGILSCIM_CACHE_FILE_HPP
#define EGILSCIM_CACHE_FILE_HPP

#include <string>
#include <fstream>
#include "model/base_object.hpp"

class object_list;

/**
 * Reads cache file specified in configuration file and
 * constructs a object list according to its contents.
 * On success, a pointer to the constructed object list is
 * returned. If the cache file doesn't exist, an empty object
 * list is returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
class cache_file {
    std::ifstream ifs;
    std::ofstream ofs;
    std::string cache_file_filename;
public:
    std::shared_ptr<object_list> get_contents();

    int save(std::shared_ptr<object_list> objects);

private:

    std::shared_ptr<object_list> read_objects();

    std::shared_ptr<base_object> read_object(std::string *uidp);

    std::shared_ptr<object_list> get_objects_from_file(const char *filename);

    int write_n(const void *buf, size_t n);

    int write_uint64(uint64_t n);

    int write_value(const std::string &av);

    int write_value_list(const std::vector<std::string> &al);

    int write_attribute(const std::string &attribute, const std::vector<std::string> &values);

    int write_object(const std::string &uid, const base_object &object);

    int read_n(void *buf, size_t n);

    int read_uint64(uint64_t *buf);

    int read_value(std::string *avp);

    int read_value_list(std::vector<std::string> *alp);

};

#endif
