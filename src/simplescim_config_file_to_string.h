#ifndef SIMPLESCIM_FILE_TO_STRING
#define SIMPLESCIM_FILE_TO_STRING

#include <stddef.h> /* size_t */

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
                                size_t file_len);

#endif
