#ifndef SIMPLESCIM_CONFIG_FILE_H
#define SIMPLESCIM_CONFIG_FILE_H

/**
 * Global string holding the current configuration file's
 * name.
 */
extern const char *simplescim_config_file_name;

/**
 * Loads configuration file 'file_name' into global
 * configuration file data structure.
 * On success, zero is returned. On error, -1 is returned
 * and 'simplescim_error_string' is set to an appropriate
 * error message.
 */
int simplescim_config_file_read(const char *file_name);

/**
 * Clears the contents of the global configuration file
 * data structure and frees any dynamically allocated
 * memory associated with it.
 */
void simplescim_config_file_clear();

/**
 * Insert key-value pair 'variable'-'value' into global
 * configuration file data structure. 'variable' and
 * 'value' must be dynamically allocated null-terminated
 * strings.
 * On success, zero is returned. On error, -1 is returned
 * and 'simplescim_error_string' is set to an appropriate
 * error message.
 */
int simplescim_config_file_insert(char *variable, char *value);

/**
 * Gets the value associated with 'variable' and assigns it
 * to 'valuep', if a value is associated with 'variable'.
 * If a value is associated with 'variable', zero is
 * returned and the value is assigned to 'valuep'.
 * Otherwise, -1 is returned and 'valuep' remains untouched.
 */
int simplescim_config_file_get(const char *variable,
                               const char **valuep);

/**
 * Performs 'func' on every 'variable'-'value' pair in the
 * global configuration file data structure.
 *
 * 'func' must have the following signature:
 * void func(const char *variable, const char *value);
 */
void simplescim_config_file_foreach(void (*func)(const char *variable,
                                                 const char *value));

#endif
