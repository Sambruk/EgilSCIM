#ifndef SIMPLESCIM_CONFIG_FILE_REQUIRED_VARIABLES_H
#define SIMPLESCIM_CONFIG_FILE_REQUIRED_VARIABLES_H

#include <stddef.h>
#include <glib.h>

/**
 * required_variables_present
 *
 * Ensures that the required variables are present in
 * simplescim_global_vars and have one of its predefined values if
 * such values exist.
 *
 * On success, zero is returned. On error, -1 is returned and an
 * error message is printed to stderr.
 */
int required_variables_present();

#endif
