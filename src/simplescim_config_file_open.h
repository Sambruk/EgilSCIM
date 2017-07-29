#ifndef SIMPLESCIM_OPEN_FILE_H
#define SIMPLESCIM_OPEN_FILE_H

#include <stddef.h>

/**
 * Opens the file 'file_name'. If 'file_sizep' is not NULL,
 * it will contain the file size of the opened file.
 * Returns the file descriptor associated with the opened
 * file, or -1 if an error occurred, in which case
 * 'simplescim_error_string' is set to an appropriate error
 * message.
 */
int simplescim_open_file(const char *file_name, size_t *file_sizep);

#endif
