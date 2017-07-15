#ifndef SIMPLESCIM_CACHE_FILE_H
#define SIMPLESCIM_CACHE_FILE_H

#include "simplescim_user_list.h"

/**
 * Reads cache file specified in global configuration file
 * structure and constructs a user list according to its
 * contents.
 * On success, a pointer to the constructed user list is
 * returned. If the cache file doesn't exist, an empty user
 * list is returned. On error, NULL is returned and
 * 'simplescim_error_string' is set to an appropriate error
 * message.
 */
struct simplescim_user_list *simplescim_cache_file_load();

#endif
