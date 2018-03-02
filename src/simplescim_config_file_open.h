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

#ifndef SIMPLESCIM_CONFIG_FILE_OPEN_H
#define SIMPLESCIM_CONFIG_FILE_OPEN_H

#include <stddef.h>

/**
 * Opens the configuration file, performs some sanity
 * checks and stores its size in 'file_sizep'.
 * Returns the configuration file's file descriptor, or -1
 * if an error occurred, in which case
 * simplescim_error_string is set to an appropriate error
 * message.
 */
int simplescim_config_file_open(size_t *file_sizep);

#endif
