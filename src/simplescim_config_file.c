#include "simplescim_config_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include "uthash.h"

#include "simplescim_error_string.h"
#include "simplescim_open_file.h"
#include "simplescim_file_to_string.h"
#include "simplescim_config_file_parser.h"
#include "simplescim_config_file_required_variables.h"


/**
 * The uthash record for the variables data structure.
 */
struct variable_record {
	char *variable;
	char *value;
	UT_hash_handle hh;
};

/**
 * A hash table representing the global configuration file
 * data structure.
 */
static struct variable_record *variables = NULL;

/**
 * Global string holding the current configuration file's
 * name.
 */
const char *simplescim_config_file_name = NULL;

/**
 * Loads configuration file 'file_name' into global
 * configuration file data structure.
 * On success, zero is returned. On error, -1 is returned
 * and 'simplescim_error_string' is set to an appropriate
 * error message.
 */
int simplescim_config_file_read(const char *file_name)
{
	int fd;
	size_t len;
	char *input;
	int err;

	/* Set global string to configuration file's name. */
	simplescim_config_file_name = file_name;

	/* Open file and get file length. */
	fd = simplescim_open_file(file_name, &len);

	if (fd == -1) {
		return -1;
	}

	/* Read file contents to string. */
	input = simplescim_file_to_string(fd, file_name, len);

	if (input == NULL) {
		close(fd);
		return -1;
	}

	close(fd);

	/* Parse file contents. */
	err = simplescim_config_file_parser(input);

	if (err == -1) {
		free(input);
		simplescim_config_file_clear();
		return -1;
	}

	free(input);

	/* Verify that all required variables are present. */
	err = simplescim_config_file_required_variables();

	if (err == -1) {
		simplescim_config_file_clear();
		return -1;
	}

	return 0;
}

/**
 * Clears the contents of the global configuration file
 * data structure and frees any dynamically allocated
 * memory associated with it.
 */
void simplescim_config_file_clear()
{
	struct variable_record *s, *tmp;

	HASH_ITER(hh, variables, s, tmp) {
		HASH_DEL(variables, s);
		free(s->variable);
		free(s->value);
		free(s);
	}

	simplescim_config_file_name = NULL;
}

/**
 * Insert key-value pair 'variable'-'value' into global
 * configuration file data structure. 'variable' and
 * 'value' must be dynamically allocated null-terminated
 * strings.
 * On success, zero is returned. On error, -1 is returned
 * and 'simplescim_error_string' is set to an appropriate
 * error message.
 */
int simplescim_config_file_insert(char *variable, char *value)
{
	struct variable_record *s;

	/* Check if 'variable' is already in the hash
	   table, in which case 'value' should replace the
	   previous value associated with 'variable' in the
	   hash table rather than be inserted. */
	HASH_FIND_STR(variables, variable, s);

	if (s == NULL) {
		/* 'variable' is not in the hash table, so
		   'value' should be inserted. */
		s = malloc(sizeof(struct variable_record));

		if (s == NULL) {
			sprintf(simplescim_error_string,
			        "%s:simplescim_config_file_insert: %s",
			        simplescim_config_file_name,
			        strerror(errno));
			return -1;
		}

		s->variable = variable;
		s->value = value;

		HASH_ADD_KEYPTR(hh,
		                variables,
		                s->variable,
		                strlen(s->variable),
		                s);

		return 0;
	}

	/* 'variable' is already in the hash table, so
	   'value' should replace the previous value
	   associated with 'variable'. 'variable' can be
	   freed since an identical dynamically allocated
	   object is already in the hash table. */

	free(variable);
	free(s->value);
	s->value = value;

	return 0;
}

/**
 * Gets the value associated with 'variable' and assigns it
 * to 'valuep', if a value is associated with 'variable'.
 * If a value is associated with 'variable', zero is
 * returned and the value is assigned to 'valuep'.
 * Otherwise, -1 is returned and 'valuep' remains untouched.
 */
int simplescim_config_file_get(const char *variable,
                               const char **valuep)
{
	struct variable_record *s;

	HASH_FIND_STR(variables, variable, s);

	if (s == NULL) {
		return -1;
	}

	*valuep = s->value;

	return 0;
}

/**
 * Performs 'func' on every 'variable'-'value' pair in the
 * global configuration file data structure.
 *
 * 'func' must have the following signature:
 * void func(const char *variable, const char *value);
 */
void simplescim_config_file_foreach(void (*func)(const char *variable,
                                                 const char *value))
{
	struct variable_record *s, *tmp;

	HASH_ITER(hh, variables, s, tmp) {
		func(s->variable, s->value);
	}
}
