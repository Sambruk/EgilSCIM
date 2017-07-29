#include "simplescim_file_to_string.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include "simplescim_error_string.h"
#include "simplescim_config_file.h"

/**
 * Reads the file contents of length 'file_len' from file
 * descriptor 'fd' into a dynamically allocated
 * null-terminated string.
 * Returns a pointer to the resulting string, or NULL if an
 * error occurred, in which case 'simplescim_error_string'
 * is set to an appropriate error message.
 */
char *simplescim_file_to_string(int fd,
                                const char *file_name,
                                size_t file_len)
{
	char *contents;
	ssize_t nread;

	contents = malloc(file_len + 1);

	if (contents == NULL) {
		sprintf(simplescim_error_string,
		        "%s:%s: %s",
		        simplescim_config_file_name,
		        file_name,
		        strerror(errno));
		return NULL;
	}

	nread = read(fd, contents, file_len);

	if (nread == -1) {
		sprintf(simplescim_error_string,
		        "%s:%s: %s",
		        simplescim_config_file_name,
		        file_name,
		        strerror(errno));
		free(contents);
		return NULL;
	}

	if ((size_t)nread < file_len) {
		sprintf(simplescim_error_string,
"%s:%s: file length is %lu B but could only read %ld B",
		        simplescim_config_file_name,
		        file_name,
		        file_len,
		        nread);
		free(contents);
		return NULL;
	}

	contents[file_len] = '\0';

	return contents;
}
