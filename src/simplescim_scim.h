#ifndef SIMPLESCIM_SCIM_H
#define SIMPLESCIM_SCIM_H

#include "simplescim_user_list.h"

/**
 * Makes SCIM requests by comparing the two user lists and
 * reading JSON templates from the configuration file.
 * Updates (or creates) the cache file.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_scim_perform(const struct simplescim_user_list *current,
                            const struct simplescim_user_list *cached);

#endif
