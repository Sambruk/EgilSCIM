#include "simplescim_config_file.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "uthash.h"

#include "simplescim_error_string.h"
#include "simplescim_config_file_open.h"
#include "simplescim_config_file_to_string.h"
#include "simplescim_config_file_parser.h"
#include "simplescim_config_file_required_variables.h"

struct variable_record {
	char *variable;
	char *value;
	UT_hash_handle hh;
};

static struct variable_record *variables = NULL;

/**
 * Global string holding the current configuration file's
 * name.
 */
const char *simplescim_config_file_name = NULL;

/**
 * Loads configuration file 'file_name'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_config_file_load(const char *file_name)
{
	int fd;
	size_t len;
	char *input;
	int err;

	/* Set global string to configuration file's name. */
	simplescim_config_file_name = file_name;

	/* Open file and get file length. */
	fd = simplescim_config_file_open(&len);

	if (fd == -1) {
		simplescim_config_file_name = NULL;
		return -1;
	}

	/* Read file contents to string. */
	input = simplescim_config_file_to_string(fd, len);

	if (input == NULL) {
		close(fd);
		simplescim_config_file_name = NULL;
		return -1;
	}

	close(fd);

	/* Parse file contents. */
	err = simplescim_config_file_parser(input);

	if (err == -1) {
		simplescim_config_file_clear();
		free(input);
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
 * Clears the loaded configuration file and frees
 * associated dynamically allocated memory.
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
 * Associates 'variable' with 'value'.
 * 'variable' and 'value' are dynamically allocated
 * null-terminated strings.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
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
		/* 'variable' is not in the hash table so
		   'value' should be inserted. */
		s = malloc(sizeof(struct variable_record));

		if (s == NULL) {
			simplescim_error_string_set_errno(
				"simplescim_config_file_insert:malloc"
			);
			return -1;
		}

		s->variable = variable;
		s->value = value;

		HASH_ADD_KEYPTR(hh,
		                variables,
		                s->variable,
		                strlen(s->variable),
		                s);
	} else {
		/* 'variable' is already in the hash table, so
		   'value' should replace the previous value
		   associated with 'variable'. 'variable' can be
		   freed since an identical dynamically allocated
		   object is already in the hash table. */
		free(variable);
		free(s->value);
		s->value = value;
	}

	return 0;
}

/**
 * Gets the value associated with 'variable' and stores it
 * in 'valuep'.
 * If 'variable' has an associated value, zero is returned.
 * Otherwise, -1 is returned.
 */
int simplescim_config_file_get(const char *variable,
                               const char **valuep)
{
	struct variable_record *s;

	HASH_FIND_STR(variables, variable, s);

	if (s == NULL) {
		return -1;
	}

	if (valuep != NULL) {
		*valuep = s->value;
	}

	return 0;
}

/**
 * Performs 'func' for every variable in the loaded
 * configuration file.
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
