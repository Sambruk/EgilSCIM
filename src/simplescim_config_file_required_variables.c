#include "simplescim_config_file_required_variables.h"

#include <stdio.h>
#include <string.h>

#include "simplescim_error_string.h"
#include "simplescim_config_file.h"

/**
 * A NULL-terminated list of variable names that must be present in a
 * configuration file. The order that the variable names appear in
 * this list is important since the lists and functions further down
 * in this file assumes that the order is consistent between lists.
 */
static const char *required_variables[] = {
	"ldap-uri",
	"ldap-who",
	"ldap-passwd",
	"ldap-base",
	"ldap-scope",
	"ldap-filter",
	"ldap-attrs",
	"ldap-attrsonly",
	"ldap-unique-identifier",
	"cache-file",
	"scim-uri",
	"scim-resource-type",
	"scim-unique-identifier",
	"scim-create",
	"scim-update",
	NULL
};

/**
 * A NULL-terminated list of possible values for the variable
 * "ldap-scope".
 */
static const char *required_values_ldap_scope[] = {
	"BASE",
	"ONELEVEL",
	"SUBTREE",
	"CHILDREN",
	NULL
};

/**
 * A NULL-terminated list of possible values for boolean variables.
 */
static const char *required_values_boolean[] = {
	"TRUE",
	"FALSE",
	NULL
};

/**
 * A list of lists that contain all valid values of a specific
 * variable. The order is assumed to be the same as in
 * required_variables, i.e. if required_variables[n] = "variable",
 * and "variable" can have value "value1" or "value2",
 * required_values[n] will point to a list of strings containing
 * {"value1", "value2", NULL}. If "variable" can have any value,
 * required_values[n] will be NULL.
 */
static const char **required_values[] = {
	NULL,                           /* ldap-uri */
	NULL,                           /* ldap-who */
	NULL,                           /* ldap-passwd */
	NULL,                           /* ldap-base */
	required_values_ldap_scope,     /* ldap-scope */
	NULL,                           /* ldap-filter */
	NULL,                           /* ldap-attrs */
	required_values_boolean,        /* ldap-attrsonly */
	NULL,                           /* ldap-unique-identifier */
	NULL,                           /* cache-file */
	NULL,                           /* scim-uri */
	NULL,                           /* scim-resource-type */
	NULL,                           /* scim-unique-identifier */
	NULL,                           /* scim-create */
	NULL                            /* scim-delete */
};

/**
 * Ensures that the required variables are present in
 * configuration file and have one of its predefined
 * values if such values are defined.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_config_file_required_variables()
{
	static char error_string[1024];
	size_t i, j;
	const char *var, *val;
	int offset;
	int err;

	for (i = 0; required_variables[i] != NULL; ++i) {
		/* Fetch required variable name */
		var = required_variables[i];

		/* Fetch the presence status
		   and value of the variable */
		err = simplescim_config_file_get(var, &val);

		if (err == -1) {
			simplescim_error_string_set_prefix(
				"%s",
				simplescim_config_file_name
			);
			simplescim_error_string_set_message(
				"required variable \"%s\" is missing",
				var
			);

			return -1;
		}

		if (required_values[i] == NULL) {
			/* Current required variable
			   can have any value */
			continue;
		}

		/* Current required variable must
		   have a value in required_values[i] */
		for (j = 0; required_values[i][j] != NULL; ++j) {
			if (strcmp(val, required_values[i][j]) == 0) {
				/* Found a match in
				   required_values[i] */
				break;
			}
		}

		if (required_values[i][j] != NULL) {
			/* Match was found, so
			   variable value is correct */
			continue;
		}

		/* No match was found, so current required
		   variable has an incorrect value */
		offset = snprintf(error_string,
		                  sizeof(error_string),
"variable \"%s\" has invalid value \"%s\"\n"
"variable \"%s\" must have one of the following values:\n",
		                  var,
		                  val,
		                  var);

		for (j = 0; required_values[i][j] != NULL; ++j) {
			if ((size_t)offset >= sizeof(error_string)) {
				break;
			}

			offset += snprintf(
				error_string + offset,
				sizeof(error_string) - offset,
				" %s",
				required_values[i][j]
			);
		}

		simplescim_error_string_set_prefix(
			"%s",
			simplescim_config_file_name
		);
		simplescim_error_string_set_message(
			"%s",
			error_string
		);

		return -1;
	}

	return 0;
}
