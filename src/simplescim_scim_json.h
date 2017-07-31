#ifndef SIMPLESCIM_SCIM_JSON_H
#define SIMPLESCIM_SCIM_JSON_H

#include "simplescim_user.h"

/**
 * Parses the input JSON template string 'json' and
 * replaces specified values with values from 'user'.
 * On success, the parsed output JSON string is returned.
 * On error, NULL is returned and simplescim_error_string
 * is set to an appropriate error message.
 */
char *simplescim_scim_json_parse(
	const char *json,
	const struct simplescim_user *user
);

#endif
