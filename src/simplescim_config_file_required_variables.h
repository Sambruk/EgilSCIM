#ifndef SIMPLESCIM_CONFIG_FILE_REQUIRED_VARIABLES_H
#define SIMPLESCIM_CONFIG_FILE_REQUIRED_VARIABLES_H

/**
 * Ensures that the required variables are present in
 * simplescim_config_file and have one of its predefined
 * values if such values exist.
 * On success, zero is returned. On error, -1 is returned
 * and 'simplescim_error_string' is set to an appropriate
 * error message.
 */
int simplescim_config_file_required_variables();

#endif
