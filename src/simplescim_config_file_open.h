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
