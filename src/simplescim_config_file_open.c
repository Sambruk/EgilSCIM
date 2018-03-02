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

#include "simplescim_config_file_open.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "simplescim_error_string.h"
#include "simplescim_config_file.h"

/**
 * Opens the configuration file, performs some sanity
 * checks and stores its size in 'file_sizep'.
 * Returns the configuration file's file descriptor, or -1
 * if an error occurred, in which case
 * simplescim_error_string is set to an appropriate error
 * message.
 */
int simplescim_config_file_open(size_t *file_sizep)
{
	int fd;
	struct stat sb;
	size_t file_size;
	ssize_t nread;
	char c;

	/* Open the file */
	fd = open(simplescim_config_file_name, O_RDONLY);

	if (fd == -1) {
		simplescim_error_string_set_errno(
			"%s",
			simplescim_config_file_name
		);
		return -1;
	}

	/* Read the file status of the file for type/mode and size */
	if (fstat(fd, &sb) == -1) {
		simplescim_error_string_set_errno(
			"%s",
			simplescim_config_file_name
		);
		close(fd);
		return -1;
	}

	/* Verify that the file is regular */
	if (!S_ISREG(sb.st_mode)) {
		simplescim_error_string_set(
			simplescim_config_file_name,
			"not a regular file"
		);
		close(fd);
		return -1;
	}

	/* Determine the actual file size */
	file_size = 0;

	while ((nread = read(fd, &c, 1)) != 0) {
		if (nread == -1) {
			simplescim_error_string_set_errno(
				"%s",
				simplescim_config_file_name
			);
			close(fd);
			return -1;
		}

		++file_size;
	}

	/* Verify that the reported file size is equal to
	   the actual file size. */
	if (file_size != (size_t)sb.st_size) {
		simplescim_error_string_set_prefix(
			"%s",
			simplescim_config_file_name
		);
		simplescim_error_string_set_message(
"reported file size %lu B is different from actual file size %lu B",
			(size_t)sb.st_size,
			file_size
		);
		close(fd);
		return -1;
	}

	/* Reposition the file to the start */
	if (lseek(fd, 0, SEEK_SET) == -1) {
		simplescim_error_string_set_errno(
			"%s",
			simplescim_config_file_name
		);
		close(fd);
		return -1;
	}

	if (file_sizep != NULL) {
		*file_sizep = file_size;
	}

	return fd;
}
