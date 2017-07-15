#include "simplescim_ldap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ldap.h>

#include "simplescim_error_string.h"
#include "simplescim_config_file.h"
#include "simplescim_ldap_attrs_parser.h"
#include "simplescim_user_list.h"
#include "simplescim_user.h"

/**
 * Global static variable holding the LDAP session.
 */
static LDAP *ld = NULL;

/**
 * Global static variable holding the LDAPsearch result.
 */
static LDAPMessage *res = NULL;

/**
 * Prints an error message concerning LDAP to
 * 'simplescim_error_string'.
 */
static void simplescim_ldap_print_error(int err, const char *func)
{
	sprintf(simplescim_error_string,
	        "%s:%s: %s",
	        simplescim_config_file_name,
	        func,
	        ldap_err2string(err));
}

/**
 * Terminates an LDAP session and frees any dynamically
 * allocated memory associated with it.
 */
static void simplescim_ldap_close()
{
	if (res != NULL) {
		/* Disregard the return value. */
		ldap_msgfree(res);
		res = NULL;
	}

	if (ld != NULL) {
		/* Disregard the return value. */
		ldap_unbind_ext(ld, NULL, NULL);
		ld = NULL;
	}
}

/**
 * Initialises LDAP session.
 */
static int simplescim_ldap_init(const char *uri,
                                const char *who,
                                const char *passwd)
{
	int ldap_version = LDAP_VERSION3;
	struct berval cred;
	int err;

	/* Initialise LDAP session */

	err = ldap_initialize(&ld, uri);

	if (err != LDAP_SUCCESS) {
		simplescim_ldap_print_error(err, "ldap_initialize");
		return -1;
	}

	/* Set protocol version */

	err = ldap_set_option(ld,
	                      LDAP_OPT_PROTOCOL_VERSION,
	                      &ldap_version);

	if (err != LDAP_OPT_SUCCESS) {
		simplescim_ldap_print_error(err, "ldap_set_option");
		simplescim_ldap_close();
		return -1;
	}

	/* Perform bind */

	cred.bv_val = (char *)passwd;
	cred.bv_len = strlen(passwd);

	err = ldap_sasl_bind_s(ld,
	                       who,
	                       LDAP_SASL_SIMPLE,
	                       &cred,
	                       NULL,
	                       NULL,
	                       NULL);

	if (err != LDAP_SUCCESS) {
		simplescim_ldap_print_error(err, "ldap_sasl_bind_s");
		simplescim_ldap_close();
		return -1;
	}

	return 0;
}

/**
 * Performs the LDAP search operation.
 */
static int simplescim_ldap_search(const char *base,
                                  const char *scope,
                                  const char *filter,
                                  const char *attrs,
                                  const char *attrsonly)
{
	int scope_val, attrsonly_val;
	char **attrs_val;
	size_t i;
	int err;

	/* Set search scope */

	if (strcmp(scope, "BASE") == 0) {
		scope_val = LDAP_SCOPE_BASE;
	} else if (strcmp(scope, "ONELEVEL") == 0) {
		scope_val = LDAP_SCOPE_ONELEVEL;
	} else if (strcmp(scope, "SUBTREE") == 0) {
		scope_val = LDAP_SCOPE_SUBTREE;
	} else {
		scope_val = LDAP_SCOPE_CHILDREN;
	}

	/* Set attrsonly */

	if (strcmp(attrsonly, "TRUE") == 0) {
		attrsonly_val = 1;
	} else {
		attrsonly_val = 0;
	}

	/* Parse attrs */

	err = simplescim_ldap_attrs_parser(attrs, &attrs_val);

	if (err == -1) {
		return -1;
	}

	/* Search */

	err = ldap_search_ext_s(
		ld,
		base,
		scope_val,
		filter,
		attrs_val,
		attrsonly_val,
		NULL,
		NULL,
		NULL,
		-1,
		&res
	);

	/* Free attrs_val if it is not NULL. */

	if (attrs_val != NULL) {
		for (i = 0; attrs_val[i] != NULL; ++i) {
			free(attrs_val[i]);
		}

		free(attrs_val);
	}

	/* Check if the search operation returned an error. */

	if (err != LDAP_SUCCESS) {
		simplescim_ldap_print_error(err, "ldap_search_ext_s");
		return -1;
	}

	return 0;
}

/**
 * Clones a 'struct berval' value into a dynamically
 * allocated object.
 * On success, a pointer to the cloned 'struct berval *' is
 * returned. On error, NULL is returned and
 * 'simplescim_error_string' is set to an appropriate error
 * message.
 */
static struct berval *clone_val(const struct berval *val)
{
	struct berval *clone;

	if (val == NULL) {
		sprintf(simplescim_error_string,
		        "%s:clone_val: cannot clone NULL\n",
		        simplescim_config_file_name);
		return NULL;
	}

	clone = malloc(sizeof(struct berval));

	if (clone == NULL) {
		simplescim_error_string_malloc();
		return NULL;
	}

	clone->bv_len = val->bv_len;
	clone->bv_val = malloc(clone->bv_len + 1);

	if (clone->bv_val == NULL) {
		simplescim_error_string_malloc();
		free(clone);
		return NULL;
	}

	memcpy(clone->bv_val, val->bv_val, clone->bv_len);
	clone->bv_val[clone->bv_len] = '\0';

	return clone;
}

/**
 * Clones a NULL-terminated list of pointers to
 * 'struct berval' values into a dynamically allocated
 * object.
 * On success, a pointer to the cloned 'struct berval **'
 * is returned. On error, NULL is returned and
 * 'simplescim_error_string' is set to an appropriate error
 * message.
 */
static struct berval **clone_vals(const struct berval **vals)
{
	struct berval **clone;
	struct berval *val;
	size_t len;
	size_t i;

	if (vals == NULL) {
		sprintf(simplescim_error_string,
		        "%s:clone_vals: cannot clone NULL\n",
		        simplescim_config_file_name);
		return NULL;
	}

	/* Calculate the length of vals */

	len = 0;

	for (i = 0; vals[i] != NULL; ++i) {
		++len;
	}

	/* Allocate the clone */

	clone = malloc(sizeof(struct berval *) * (len + 1));

	if (clone == NULL) {
		simplescim_error_string_malloc();
		return NULL;
	}

	/* Clone and insert all values */

	for (i = 0; i < len; ++i) {
		/* Clone value i */

		val = clone_val(vals[i]);

		if (val == NULL) {
			while (i > 0) {
				free(clone[i - 1]->bv_val);
				free(clone[i - 1]);
				--i;
			}

			free(clone);

			return NULL;
		}

		/* Insert the value */

		clone[i] = val;
	}

	clone[len] = NULL;

	return clone;
}

/**
 * Converts an entry in the LDAP search results into a user.
 * On success, a pointer to a new user object is returned.
 * On error, NULL is returned and 'simplescim_error_string'
 * is set to an appropriate error message.
 */
static struct simplescim_user *entry_to_user(LDAPMessage *entry)
{
	struct simplescim_user *user;
	char *attr, *attr_clone;
	struct berval **vals, **vals_clone;
	BerElement *ber;
	size_t i;
	int err;

	/* Create the user object. */

	user = simplescim_user_new();

	for (attr = ldap_first_attribute(ld, entry, &ber);
	     attr != NULL;
	     attr = ldap_next_attribute(ld, entry, ber)) {
		/* Clone 'attr'. */
		attr_clone = strdup(attr);

		if (attr_clone == NULL) {
			simplescim_error_string_malloc();

			simplescim_user_delete(user);

			ldap_memfree(attr);
			ber_free(ber, 0);

			return NULL;
		}

		/* Get and clone 'vals'. */
		vals = ldap_get_values_len(ld, entry, attr);

		if (vals == NULL) {
			sprintf(simplescim_error_string,
			        "%s:ldap_get_values_len: unknown error",
			        simplescim_config_file_name);

			free(attr_clone);
			simplescim_user_delete(user);

			ldap_memfree(attr);
			ber_free(ber, 0);

			return NULL;
		}

		vals_clone = clone_vals((const struct berval **)vals);
 
		if (vals_clone == NULL) {
			free(attr_clone);
			simplescim_user_delete(user);

			ldap_value_free_len(vals);
			ldap_memfree(attr);
			ber_free(ber, 0);

			return NULL;
		}

		/* Set attribute in user object */

		err = simplescim_user_set_attribute(user,
		                                    attr_clone,
		                                    vals_clone);

		if (err == -1) {
			for (i = 0; vals_clone[i] != NULL; ++i) {
				free(vals_clone[i]->bv_val);
				free(vals_clone[i]);
			}

			free(vals_clone);
			free(attr_clone);
			simplescim_user_delete(user);

			ldap_value_free_len(vals);
			ldap_memfree(attr);
			ber_free(ber, 0);

			return NULL;
		}

		/* Free LDAP data */

		ldap_value_free_len(vals);
		ldap_memfree(attr);
	}

	ber_free(ber, 0);

	return user;
}

/**
 * Returns a clone of the 'struct berval *' value with
 * attribute 'unique_identifier_attr', if such exists.
 * On success, a pointer to the cloned object is returned.
 * On error, NULL is returned and 'simplescim_error_string'
 * is set to an appropriate error message.
 */
static struct berval *
get_users_unique_identifier(struct simplescim_user *user,
                            const char *unique_identifier_attr)
{
	const struct berval **vals;
	struct berval *unique_identifier;
	int err;

	err = simplescim_user_get_attribute(
		user,
		unique_identifier_attr,
		&vals
	);

	if (err == -1) {
		sprintf(simplescim_error_string,
		        "%s:LDAP: user does not have attribute \"%s\"",
		        simplescim_config_file_name,
		        unique_identifier_attr);
		return NULL;
	}

	if (vals[0] == NULL || vals[1] != NULL) {
		sprintf(simplescim_error_string,
"%s:LDAP: attribute \"%s\" must have exactly one value",
		        simplescim_config_file_name,
		        unique_identifier_attr);
		return NULL;
	}

	unique_identifier = clone_val(vals[0]);

	if (unique_identifier == NULL) {
		return NULL;
	}

	return unique_identifier;
}

/**
 * Construct the user list object from the LDAP response.
 * On success, a pointer to the constructed object is
 * returned. On error, NULL is returned and
 * 'simplescim_error_string' is set to an appropriate
 * error message.
 */
static struct simplescim_user_list *simplescim_ldap_to_user_list()
{
	struct simplescim_user_list *users;
	struct simplescim_user *user;
	struct berval *unique_identifier;
	const char *unique_identifier_attr;
	LDAPMessage *entry;
	size_t entry_n;
	int err;

	/* Initialise user list */

	users = simplescim_user_list_new();

	/* Fetch which LDAP attribute is the unique
	   identifier. */

	simplescim_config_file_get("ldap-unique-identifier",
	                           &unique_identifier_attr);

	/* For every response entry, create a user and
	   insert it into the user list. */

	entry_n = 1;

	for (entry = ldap_first_entry(ld, res);
	     entry != NULL;
	     entry = ldap_next_entry(ld, entry)) {

		/* Create the user. */

		user = entry_to_user(entry);

		if (user == NULL) {
			simplescim_user_list_delete(users);
			return NULL;
		}

		/* Get the user's unique identifier. */

		unique_identifier = get_users_unique_identifier(
			user,
			unique_identifier_attr
		);

		if (unique_identifier == NULL) {
			simplescim_user_delete(user);
			simplescim_user_list_delete(users);
			return NULL;
		}

		/* Insert user into user list. */

		err = simplescim_user_list_insert_user(
			users,
			unique_identifier,
			user
		);

		if (err == -1) {
			free(unique_identifier->bv_val);
			free(unique_identifier);
			simplescim_user_delete(user);
			simplescim_user_list_delete(users);
			return NULL;
		}

		++entry_n;
	}

	return users;
}

/**
 * Reads user data from LDAP into a user list object using
 * the global configuration file structure and returns a
 * pointer it. On error, NULL is returned and
 * 'simplescim_error_string' is set to an appropriate error
 * message.
 */
struct simplescim_user_list *simplescim_ldap_read()
{
	struct simplescim_user_list *users;
	const char *uri, *who, *passwd;
	const char *base, *scope, *filter;
	const char *attrs, *attrsonly;
	int err;

	/* Initialise LDAP session. */

	uri = who = passwd = NULL;

	simplescim_config_file_get("ldap-uri", &uri);
	simplescim_config_file_get("ldap-who", &who);
	simplescim_config_file_get("ldap-passwd", &passwd);

	err = simplescim_ldap_init(uri, who, passwd);

	if (err == -1) {
		return NULL;
	}

	/* Perform search operation. */

	base = scope = filter = attrs = attrsonly = NULL;

	simplescim_config_file_get("ldap-base", &base);
	simplescim_config_file_get("ldap-scope", &scope);
	simplescim_config_file_get("ldap-filter", &filter);
	simplescim_config_file_get("ldap-attrs", &attrs);
	simplescim_config_file_get("ldap-attrsonly", &attrsonly);

	err = simplescim_ldap_search(base,
	                             scope,
	                             filter,
	                             attrs,
	                             attrsonly);

	if (err == -1) {
		simplescim_ldap_close();
		return NULL;
	}

	/* Create user list structure. */

	users = simplescim_ldap_to_user_list();

	if (users == NULL) {
		simplescim_ldap_close();
		return NULL;
	}

	/* Close LDAP session */

	simplescim_ldap_close();

	return users;
}
