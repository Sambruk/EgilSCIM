/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2021 Föreningen Sambruk
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

#ifndef EGILSCIM_RENDERED_CACHE_FILE_HPP
#define EGILSCIM_RENDERED_CACHE_FILE_HPP

#include <stdexcept>
#include <memory>
#include "model/rendered_object_list.hpp"

namespace rendered_cache_file {

class bad_format : public std::runtime_error {
public:
    bad_format() : std::runtime_error("unrecognized file format") {}
};

/**
 * Reads cache file and constructs an object list according to its contents.
 * On success, the constructed object list is returned. If the cache file 
 * doesn't exist, an empty object list is returned. 
 * 
 * On error, an std::runtime_error is thrown.
 */
std::shared_ptr<rendered_object_list> get_contents(const std::string& path);

/**
  * This will prepare a temporary file (next to the file pointed to by path) open it
  * and preallocate space so that we can be confident that we don't run out of space
  * when we're writing the new cache content.
  * 
  * ofs will point to the start of this temporary file after this call.
  * 
  * On error, an std::runtime_error is thrown.
  */
void begin_rendered_cache_file(const std::string& path, size_t size_to_pre_allocate, std::ofstream& ofs);

/**
 * Writes the contents for a new cache file based on an object list.
 * ofs should point to the start of a file to write to. After successful
 * writes ofs should point to the end of the new cache file.
 *
 * On error, an std::runtime_error is thrown.
 */
void save(std::ofstream& ofs, std::shared_ptr<rendered_object_list> objects);

/**
  * This should be called after a successful save, to finalize the writing of the new
  * cache file. Note that ofs should then point to the end of the temporary file we just
  * wrote, and path should specify the path to the actual cache file (not the temporary).
  * 
  * This function will close ofs and resize the file (assuming ofs is pointing to the
  * end of the cache file when this function is called). Then the temporary file will
  * be renamed to the actual cache file path.
  * 
  * On error, an std::runtime_error is thrown.
  */
void finalize_rendered_cache_file(std::ofstream& ofs, const std::string& path);

/**
 * Estimate needed size of the new cache file based on the current objects and the cached objects.
 * 
 * It will calculate an upper estimate so that we can be sure that the new cache file will not be
 * larger than the estimate.
 * 
 * Note that the function is called before we've started doing SCIM operations (we want to make
 * sure we have enough disk space before we start sending anything), so we won't know how many
 * of the operations will succeed.
 */
size_t size_estimate(const rendered_object_list& current_objects, const rendered_object_list& cached);

}

#endif // EGILSCIM_RENDERED_CACHE_FILE_HPP
