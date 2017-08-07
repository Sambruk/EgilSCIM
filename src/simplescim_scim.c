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

static const char *simplescim_scim_uri;
static const char *simplescim_scim_resource_type;
static const char *simplescim_scim_unique_identifier;
static const char *simplescim_scim_create;
static const char *simplescim_scim_update;
static struct simplescim_user_list *simplescim_scim_new_cache;

static int simplescim_scim_init()
{
	/* Fetch variables from configuration file */
	simplescim_config_file_get("scim-uri",
	                           &simplescim_scim_uri);
	simplescim_config_file_get("scim-resource-type",
	                           &simplescim_scim_resource_type);
	simplescim_config_file_get("scim-unique-identifier",
	                           &simplescim_scim_unique_identifier);
	simplescim_config_file_get("scim-create",
	                           &simplescim_scim_create);
	simplescim_config_file_get("scim-update",
	                           &simplescim_scim_update);

	/* Allocate new cache */
	simplescim_scim_new_cache = simplescim_user_list_new();

	if (simplescim_scim_new_cache == NULL) {
		return -1;
	}

	/* Initialise connection */
	/* TODO: Initialise libcurl */

	return 0;
}

static void simplescim_scim_clear()
{
	/* Close connection */
	/* TODO: Close libcurl */

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
	struct simplescim_user *copied_user;
	struct simplescim_arbval *uid;
	struct simplescim_arbval *scim_id;
	struct simplescim_arbval_list *scim_id_list;
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

	/* TODO: Construct SCIM POST request with scim-uri,
	         scim-resource-type and jobj and send
	         request with libcurl */

	/* TODO: Get SCIM response and get scim-id using
	         scim-unique-identifier */

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
	scim_id = simplescim_arbval_string("placeholder-scim-id");

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
		strdup("scim-id"),
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

	/* Print SCIM request */
	printf("==== Simulated request ====\n");
	printf("POST %s HTTP/1.1\n", simplescim_scim_resource_type);
	printf("Host: %s\n", simplescim_scim_uri);
	printf("Accept: application/scim+json\n");
	printf("Content-Type: application/scim+json\n");
	printf("Content-Length: ...\n\n");
	printf(
		"%s\n",
		json_object_to_json_string_ext(
			jobj,
			JSON_C_TO_STRING_SPACED
			| JSON_C_TO_STRING_PRETTY
		)
	);

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
	struct simplescim_user *copied_user;
	struct simplescim_arbval *uid;
	const struct simplescim_arbval_list *scim_id;
	struct simplescim_arbval_list *scim_id_copy;
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
		"scim-id",
		&scim_id
	);

	if (err == -1) {
		simplescim_error_string_set(
			"update_user_func",
			"cached user does not have attribute "
			"\"scim-id\""
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
		strdup("scim-id"),
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

	/* TODO: Construct SCIM PUT request with scim-uri,
	         scim-resource-type, scim_id and jobj and
	         send request with libcurl */

	/* TODO: Get SCIM response */

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

	/* Print SCIM request */
	printf("==== Simulated request ====\n");
	printf("PUT %s/%s\n",
	       simplescim_scim_resource_type,
	       scim_id->al_vals[0]->av_val);
	printf("Host: %s\n", simplescim_scim_uri);
	printf("Accept: application/scim+json\n");
	printf("Content-Type: application/scim+json\n\n");
	printf(
		"%s\n",
		json_object_to_json_string_ext(
			jobj,
			JSON_C_TO_STRING_SPACED
			| JSON_C_TO_STRING_PRETTY
		)
	);

	/* Clean up */
	json_object_put(jobj);

	return 0;
}

static int delete_user_func(const struct simplescim_user *cached_user)
{
	const struct simplescim_arbval_list *scim_id;
	int err;

	/* Get SCIM ID */
	err = simplescim_user_get_attribute(
		cached_user,
		"scim-id",
		&scim_id
	);

	if (err == -1) {
		simplescim_error_string_set(
			"delete_user_func",
			"cached user does not have attribute "
			"\"scim-id\""
		);
		return -1;
	}

	/* TODO: Construct SCIM DELETE request with
	         scim-uri, scim-resource-type and scim_id
	         and send request with libcurl */

	/* TODO: Get SCIM response */

	/* Print SCIM request */
	printf("==== Simulated request ====\n");
	printf("DELETE %s/%s\n",
	       simplescim_scim_resource_type,
	       scim_id->al_vals[0]->av_val);
	printf("Host: %s\n\n", simplescim_scim_uri);

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
