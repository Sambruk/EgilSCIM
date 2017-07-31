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
char *simplescim_config_file_to_string(int fd,
                                       size_t file_size);

#endif
