#include "simplescim_open_file.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "simplescim_error_string.h"
#include "simplescim_config_file.h"

/**
 * Opens the file 'file_name'. If 'file_sizep' is not NULL,
 * it will contain the file size of the opened file.
 * Returns the file descriptor associated with the opened
 * file, or -1 if an error occurred, in which case
 * 'simplescim_error_string' is set to an appropriate error
 * message.
 */
int simplescim_open_file(const char *file_name, size_t *file_sizep)
{
	int fd;
	struct stat sb;
	ssize_t nread;
	size_t file_size;
	char c;

	/**
	 * Open the file.
	 */

	fd = open(file_name, O_RDONLY);

	if (fd == -1) {
		sprintf(simplescim_error_string,
		        "%s:%s: %s",
		        simplescim_config_file_name,
		        file_name,
		        strerror(errno));
		return -1;
	}

	/**
	 * Stat the file for type/mode and size.
	 */

	if (fstat(fd, &sb) == -1) {
		sprintf(simplescim_error_string,
		        "%s:%s: %s",
		        simplescim_config_file_name,
		        file_name,
		        strerror(errno));
		close(fd);
		return -1;
	}

	/**
	 * Verify that the file is regular.
	 */

	if (!S_ISREG(sb.st_mode)) {
		sprintf(simplescim_error_string,
		        "%s:%s: not a regular file",
		        simplescim_config_file_name,
		        file_name);
		close(fd);
		return -1;
	}

	/**
	 * If 'file_sizep' is NULL, the caller doesn't care
	 * about the size of the file, so the file
	 * descriptor can now be returned.
	 */

	if (file_sizep == NULL) {
		return fd;
	}

	/**
	 * Determine the actual file size.
	 */

	file_size = 0;

	for (;;) {
		nread = read(fd, &c, 1);

		if (nread == -1) {
			sprintf(simplescim_error_string,
				"%s:%s: %s",
		        	simplescim_config_file_name,
				file_name,
				strerror(errno));
			close(fd);
			return -1;
		}

		if (nread == 0) {
			break;
		}

		++file_size;
	}

	/**
	 * Verify that the reported file size is equal to
	 * the actual file size.
	 */

	if (file_size != (size_t)sb.st_size) {
		sprintf(simplescim_error_string,
"%s:%s: reported file size %lu B is different from actual file size %lu\n",
		        simplescim_config_file_name,
		        file_name,
		        sb.st_size,
		        file_size);
		close(fd);
		return -1;
	}

	/**
	 * Return the file descriptor and the file size.
	 * The file must be repositioned to the start.
	 */

	if (lseek(fd, 0, SEEK_SET) == -1) {
		sprintf(simplescim_error_string,
		        "%s:%s: %s",
		        simplescim_config_file_name,
		        file_name,
		        strerror(errno));
		close(fd);
		return -1;
	}

	*file_sizep = file_size;

	return fd;
}
