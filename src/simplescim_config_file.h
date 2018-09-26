/**
 * Copyright © 2017-2018  Max Wällstedt <>
 *
 * This file is part of SimpleSCIM.
 *
 * SimpleSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with SimpleSCIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SIMPLESCIM_CONFIG_FILE_H
#define SIMPLESCIM_CONFIG_FILE_H

/**
 * Global string holding the current configuration file's
 * name.
 */
extern const char *simplescim_config_file_name;

/**
 * Loads configuration file 'file_name'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_config_file_load(
	const char *file_name
);

/**
 * Clears the loaded configuration file and frees
 * associated dynamically allocated memory.
 */
void simplescim_config_file_clear();

/**
 * Associates 'variable' with 'value'.
 * 'variable' and 'value' are dynamically allocated
 * null-terminated strings.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_config_file_insert(
	char *variable,
	char *value
);

/**
 * Gets the value associated with 'variable' and stores it
 * in 'valuep'.
 * If 'variable' has an associated value, zero is returned.
 * Otherwise, -1 is returned.
 */
int simplescim_config_file_get(
	const char *variable,
	const char **valuep
);

/**
 * Gets the value associated with 'variable' and stores it
 * in 'valuep' unless 'valuep' is NULL.
 * If 'variable' has an associated value, zero is returned.
 * Otherwise, -1 is returned and simplescim_error_string is
 * set to an appropriate error message.
 */
int simplescim_config_file_require(
	const char *variable,
	const char **valuep
);

/**
 * Performs 'func' for every variable in the loaded
 * configuration file.
 * 'func' must have the following signature:
 * void func(const char *variable, const char *value);
 */
void simplescim_config_file_foreach(
	void (*func)(const char *variable, const char *value)
);

#endif
