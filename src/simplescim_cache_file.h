/**
 * Copyright © 2017-2018  Max Wällstedt <max.wallstedt@gmail.com>
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

#include "simplescim_user_list.h"

/**
 * Reads cache file specified in configuration file and
 * constructs a user list according to its contents.
 * On success, a pointer to the constructed user list is
 * returned. If the cache file doesn't exist, an empty user
 * list is returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
struct simplescim_user_list *simplescim_cache_file_get_users();

struct simplescim_user_list *simplescim_cache_file_get_users_from_file(
	const char *filename
);

/**
 * Writes 'users' to cache file specified in configuration
 * file.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_cache_file_save(const struct simplescim_user_list *users);

#endif
