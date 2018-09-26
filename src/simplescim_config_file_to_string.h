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

#ifndef SIMPLESCIM_CONFIG_FILE_TO_STRING
#define SIMPLESCIM_CONFIG_FILE_TO_STRING

#include <stddef.h>

/**
 * Reads the file contents of the configuration file of
 * size 'file_size' from file descriptor 'fd' into a
 * dynamically allocated null-terminated string.
 * Returns a pointer to the resulting string, or NULL if an
 * error occurred, in which case simplescim_error_string is
 * set to an appropriate error message.
 */
char *simplescim_config_file_to_string(
	int fd,
	size_t file_size
);

#endif
