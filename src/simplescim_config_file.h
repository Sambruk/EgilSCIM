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
int simplescim_config_file_load(const char *file_name);

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
int simplescim_config_file_insert(char *variable, char *value);

/**
 * Gets the value associated with 'variable' and stores it
 * in 'valuep'.
 * If 'variable' has an associated value, zero is returned.
 * Otherwise, -1 is returned.
 */
int simplescim_config_file_get(const char *variable,
                               const char **valuep);

/**
 * Performs 'func' for every variable in the loaded
 * configuration file.
 * 'func' must have the following signature:
 * void func(const char *variable, const char *value);
 */
void simplescim_config_file_foreach(void (*func)(const char *variable,
                                                 const char *value));

#endif
