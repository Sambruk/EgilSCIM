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

#include "simplescim_scim.h"

#include <stdio.h>
#include <string.h>
#include <json.h>

#include "simplescim_error_string.h"
#include "simplescim_arbval.h"
#include "simplescim_arbval_list.h"
#include "simplescim_user.h"
#include "simplescim_user_list.h"
#include "simplescim_config_file.h"
#include "simplescim_cache_file.h"
#include "simplescim_scim_json.h"
#include "simplescim_scim_send.h"

struct var_ent {
	const char *var;
	const char **dest;
};

static const char *simplescim_cert;
static const char *simplescim_key;
static const char *simplescim_pinnedpubkey;
static const char *simplescim_scim_url;
static const char *simplescim_scim_resource_identifier;
static const char *simplescim_scim_create;
static const char *simplescim_scim_update;
static const char *simplescim_user_scim_resource_identifier;
static struct simplescim_user_list *simplescim_scim_new_cache;

static int simplescim_scim_get_variables()
{
	struct var_ent variables[] = {
		{"cert",
		 &simplescim_cert},
		{"key",
		 &simplescim_key},
		{"pinnedpubkey",
		 &simplescim_pinnedpubkey},
		{"scim-url",
		 &simplescim_scim_url},
		{"scim-resource-identifier",
		 &simplescim_scim_resource_identifier},
		{"scim-create",
		 &simplescim_scim_create},
		{"scim-update",
		 &simplescim_scim_update},
		{"user-scim-resource-identifier",
		 &simplescim_user_scim_resource_identifier}
	};
	size_t n_variables;
	size_t i;
	int err;

	n_variables = sizeof(variables) / sizeof(struct var_ent);

	for (i = 0; i < n_variables; ++i) {
		err = simplescim_config_file_require(
			variables[i].var,
			variables[i].dest
		);

		if (err == -1) {
			return -1;
		}
	}

	return 0;
}

static int simplescim_scim_init()
{
	int err;

	/* Fetch variables from configuration file */
	err = simplescim_scim_get_variables();

	if (err == -1) {
		return -1;
	}

	/* Allocate new cache */
	simplescim_scim_new_cache = simplescim_user_list_new();

	if (simplescim_scim_new_cache == NULL) {
		return -1;
	}

	/* Initialise simplescim_scim_send */
	err = simplescim_scim_send_init(
		simplescim_cert,
		simplescim_key,
		simplescim_pinnedpubkey
	);

	if (err == -1) {
		simplescim_user_list_delete(simplescim_scim_new_cache);
		return -1;
	}

	return 0;
}

static void simplescim_scim_clear()
{
	/* Clear simplescim_scim_send */
	simplescim_scim_send_clear();

	/* Delete new cache */
	simplescim_user_list_delete(simplescim_scim_new_cache);
}

static int copy_user_func(const struct simplescim_user *cached_user)
{
	struct simplescim_user *copied_user;
	struct simplescim_arbval *uid;
	int err;

	/* Copy user */
	copied_user = simplescim_user_copy(cached_user);

	if (copied_user == NULL) {
		return -1;
	}

	uid = simplescim_user_get_uid(copied_user);

	if (uid == NULL) {
		simplescim_user_delete(copied_user);
		return -1;
	}

	/* Insert copied user into new cache */
	err = simplescim_user_list_insert_user(
		simplescim_scim_new_cache,
		uid,
		copied_user
	);

	if (err == -1) {
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		return -1;
	}

	return 0;
}

static int create_user_func(const struct simplescim_user *user)
{
	char *parsed_json;
	char *response_json;
	struct simplescim_user *copied_user;
	struct simplescim_arbval *uid;
	struct simplescim_arbval *scim_id;
	struct simplescim_arbval_list *scim_id_list;
	struct json_object *scim_id_obj;
	const char *scim_id_str;
	struct json_object *jobj;
	enum json_tokener_error jerr;
	int err;

	/* Create JSON object for user */

	parsed_json = simplescim_scim_json_parse(
		simplescim_scim_create,
		user
	);

	if (parsed_json == NULL) {
		return -1;
	}

	/* Verify JSON string */

	jobj = json_tokener_parse_verbose(parsed_json, &jerr);

	if (jobj == NULL) {
		simplescim_error_string_set_prefix(
			"create_user_func:"
			"json_tokener_parse_verbose"
		);
		simplescim_error_string_set_message(
			"%s",
			json_tokener_error_desc(jerr)
		);
		free(parsed_json);
		return -1;
	}

	free(parsed_json);

	/* Send SCIM create request */

	err = simplescim_scim_send_create(
		simplescim_scim_url,
		json_object_to_json_string_ext(
			jobj,
			JSON_C_TO_STRING_SPACED
			| JSON_C_TO_STRING_PRETTY
		),
		(const char **)&response_json
	);

	if (err == -1) {
		json_object_put(jobj);
		return -1;
	}

	json_object_put(jobj);

	/* Get SCIM resource identifier */

	jobj = json_tokener_parse_verbose(response_json, &jerr);

	if (jobj == NULL) {
		simplescim_error_string_set_prefix(
			"create_user_func:"
			"json_tokener_parse_verbose"
		);
		simplescim_error_string_set_message(
			"%s",
			json_tokener_error_desc(jerr)
		);
		free(response_json);
		return -1;
	}

	free(response_json);

	if (!json_object_object_get_ex(
		jobj,
		simplescim_scim_resource_identifier,
		&scim_id_obj
	)) {
		simplescim_error_string_set_prefix(
			"create_user_func:"
			"json_object_object_get_ex"
		);
		simplescim_error_string_set_message(
			"variable \"%s\" is not present in JSON response",
			simplescim_scim_resource_identifier
		);
		json_object_put(jobj);
		return -1;
	}

	if (!json_object_is_type(scim_id_obj, json_type_string)) {
		simplescim_error_string_set_prefix(
			"create_user_func:"
			"json_object_is_type"
		);
		simplescim_error_string_set_message(
			"variable \"%s\" is not a string in JSON response",
			simplescim_scim_resource_identifier
		);
		json_object_put(jobj);
		return -1;
	}

	scim_id_str = json_object_get_string(scim_id_obj);

	/* Copy user */
	copied_user = simplescim_user_copy(user);

	if (copied_user == NULL) {
		json_object_put(jobj);
		return -1;
	}

	uid = simplescim_user_get_uid(copied_user);

	if (uid == NULL) {
		simplescim_user_delete(copied_user);
		json_object_put(jobj);
		return -1;
	}

	/* Insert SCIM ID */
	scim_id = simplescim_arbval_string(scim_id_str);

	if (scim_id == NULL) {
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		json_object_put(jobj);
		return -1;
	}

	scim_id_list = simplescim_arbval_list_new(1);

	if (scim_id_list == NULL) {
		simplescim_arbval_delete(scim_id);
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		json_object_put(jobj);
		return -1;
	}

	err = simplescim_arbval_list_append(
		scim_id_list,
		scim_id
	);

	if (err == -1) {
		simplescim_arbval_list_delete(scim_id_list);
		simplescim_arbval_delete(scim_id);
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		json_object_put(jobj);
		return -1;
	}

	err = simplescim_user_set_attribute(
		copied_user,
		strdup(simplescim_user_scim_resource_identifier),
		scim_id_list
	);

	if (err == -1) {
		simplescim_arbval_list_delete(scim_id_list);
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		json_object_put(jobj);
		return -1;
	}

	/* Insert copied user into new cache */
	err = simplescim_user_list_insert_user(
		simplescim_scim_new_cache,
		uid,
		copied_user
	);

	if (err == -1) {
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		json_object_put(jobj);
		return -1;
	}

	/* Clean up */
	json_object_put(jobj);

	return 0;
}

static int update_user_func(
	const struct simplescim_user *user,
	const struct simplescim_user *cached_user
)
{
	char *parsed_json;
	char *response_json;
	struct simplescim_user *copied_user;
	struct simplescim_arbval *uid;
	const struct simplescim_arbval_list *scim_id;
	struct simplescim_arbval_list *scim_id_copy;
	char *url;
	size_t url_len;
	struct json_object *jobj;
	enum json_tokener_error jerr;
	int err;

	/* Copy user */
	copied_user = simplescim_user_copy(user);

	if (copied_user == NULL) {
		return -1;
	}

	uid = simplescim_user_get_uid(copied_user);

	if (uid == NULL) {
		simplescim_user_delete(copied_user);
		return -1;
	}

	/* Get SCIM ID */
	err = simplescim_user_get_attribute(
		cached_user,
		simplescim_user_scim_resource_identifier,
		&scim_id
	);

	if (err == -1) {
		simplescim_error_string_set_prefix(
			"update_user_func:"
			"simplescim_user_get_attribute"
		);
		simplescim_error_string_set_message(
			"cached user does not have attribute \"%s\"",
			simplescim_user_scim_resource_identifier
		);
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		return -1;
	}

	/* Insert SCIM ID */
	scim_id_copy = simplescim_arbval_list_copy(scim_id);

	if (scim_id_copy == NULL) {
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		return -1;
	}

	err = simplescim_user_set_attribute(
		copied_user,
		strdup(simplescim_user_scim_resource_identifier),
		scim_id_copy
	);

	if (err == -1) {
		simplescim_arbval_list_delete(scim_id_copy);
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		return -1;
	}

	/* Create JSON object for user */
	parsed_json = simplescim_scim_json_parse(
		simplescim_scim_update,
		copied_user
	);

	if (parsed_json == NULL) {
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		return -1;
	}

	/* Verify JSON string */
	jobj = json_tokener_parse_verbose(parsed_json, &jerr);

	if (jobj == NULL) {
		simplescim_error_string_set_prefix(
			"update_user_func:"
			"json_tokener_parse_verbose"
		);
		simplescim_error_string_set_message(
			"%s",
			json_tokener_error_desc(jerr)
		);
		free(parsed_json);
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		return -1;
	}

	free(parsed_json);

	/* Send SCIM update request */

	url_len = strlen(simplescim_scim_url)
	          + scim_id_copy->al_vals[0]->av_len
	          + 1;
	url = malloc(url_len + 1);

	if (url == NULL) {
		simplescim_error_string_set_errno(
			"update_user_func:"
			"malloc"
		);
		json_object_put(jobj);
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		return -1;
	}

	strcpy(url, simplescim_scim_url);
	strcat(url, "/");
	strcat(url, (const char *)scim_id_copy->al_vals[0]->av_val);

	err = simplescim_scim_send_update(
		url,
		json_object_to_json_string_ext(
			jobj,
			JSON_C_TO_STRING_SPACED
			| JSON_C_TO_STRING_PRETTY
		),
		(const char **)&response_json
	);

	if (err == -1) {
		free(url);
		json_object_put(jobj);
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		return -1;
	}

	free(response_json);
	free(url);
	json_object_put(jobj);

	/* Insert copied user into new cache */

	err = simplescim_user_list_insert_user(
		simplescim_scim_new_cache,
		uid,
		copied_user
	);

	if (err == -1) {
		simplescim_arbval_delete(uid);
		simplescim_user_delete(copied_user);
		return -1;
	}

	return 0;
}

static int delete_user_func(const struct simplescim_user *cached_user)
{
	const struct simplescim_arbval_list *scim_id;
	char *url;
	size_t url_len;
	int err;

	/* Get SCIM ID */
	err = simplescim_user_get_attribute(
		cached_user,
		simplescim_user_scim_resource_identifier,
		&scim_id
	);

	if (err == -1) {
		simplescim_error_string_set_prefix(
			"delete_user_func:"
			"simplescim_user_get_attribute"
		);
		simplescim_error_string_set_message(
			"cached user does not have attribute \"%s\"",
			simplescim_user_scim_resource_identifier
		);
		return -1;
	}

	/* Send SCIM delete request */

	url_len = strlen(simplescim_scim_url)
	          + scim_id->al_vals[0]->av_len
	          + 1;
	url = malloc(url_len + 1);

	if (url == NULL) {
		simplescim_error_string_set_errno(
			"delete_user_func:"
			"malloc"
		);
		return -1;
	}

	strcpy(url, simplescim_scim_url);
	strcat(url, "/");
	strcat(url, (const char *)scim_id->al_vals[0]->av_val);

	err = simplescim_scim_send_delete(
		url
	);

	if (err == -1) {
		free(url);
		return -1;
	}

	free(url);

	return 0;
}

/**
 * Makes SCIM requests by comparing the two user lists and
 * reading JSON templates from the configuration file.
 * Updates (or creates) the cache file.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_scim_perform(
	const struct simplescim_user_list *current,
	const struct simplescim_user_list *cached
)
{
	int err;

	/* Initialise SCIM */
	err = simplescim_scim_init();

	if (err == -1) {
		return -1;
	}

	/* Find all changes and perform necessary operations */
	err = simplescim_user_list_find_changes(
		current,
		cached,
		copy_user_func,
		create_user_func,
		update_user_func,
		delete_user_func
	);

	if (err == -1) {
		simplescim_scim_clear();
		return -1;
	}

	/* Save new cache file */
	err = simplescim_cache_file_save(simplescim_scim_new_cache);

	if (err == -1) {
		simplescim_scim_clear();
		return -1;
	}

	/* Clean up */
	simplescim_scim_clear();

	return 0;
}
