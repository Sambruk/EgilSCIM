#include "simplescim_ldap.h"

#include <stdlib.h>
#include <string.h>
#include <ldap.h>

#include "simplescim_error_string.h"
#include "simplescim_arbval.h"
#include "simplescim_arbval_list.h"
#include "simplescim_user.h"
#include "simplescim_user_list.h"
#include "simplescim_config_file.h"
#include "simplescim_ldap_attrs_parser.h"

struct var_ent {
	const char *var;
	const char **dest;
};

/**
 * LDAP state variables
 */

static LDAP *simplescim_ldap_ld = NULL;
static LDAPMessage *simplescim_ldap_res = NULL;

/**
 * Configuration file variables
 */

static const char *simplescim_ldap_uri;
static const char *simplescim_ldap_who;
static const char *simplescim_ldap_passwd;
static const char *simplescim_ldap_base;
static const char *simplescim_ldap_scope;
static const char *simplescim_ldap_filter;
static const char *simplescim_ldap_attrs;
static const char *simplescim_ldap_attrsonly;

/**
 * Prints an error message concerning LDAP to
 * simplescim_error_string.
 */
static void simplescim_ldap_print_error(int err, const char *func)
{
	simplescim_error_string_set_prefix(
		"%s",
		func
	);
	simplescim_error_string_set_message(
		"%s",
		ldap_err2string(err)
	);
}

/**
 * Terminates an LDAP session and frees any dynamically
 * allocated memory associated with it.
 */
static void simplescim_ldap_close()
{
	if (simplescim_ldap_res != NULL) {
		/* Disregard the return value. */
		ldap_msgfree(simplescim_ldap_res);
		simplescim_ldap_res = NULL;
	}

	if (simplescim_ldap_ld != NULL) {
		/* Disregard the return value. */
		ldap_unbind_ext(simplescim_ldap_ld, NULL, NULL);
		simplescim_ldap_ld = NULL;
	}
}

/**
 * Gets and verifies all LDAP variables from the configuration file.
 */
int simplescim_ldap_get_variables()
{
	struct var_ent variables[] = {
		{"ldap-uri",
		 &simplescim_ldap_uri},
		{"ldap-who",
		 &simplescim_ldap_who},
		{"ldap-passwd",
		 &simplescim_ldap_passwd},
		{"ldap-base",
		 &simplescim_ldap_base},
		{"ldap-scope",
		 &simplescim_ldap_scope},
		{"ldap-filter",
		 &simplescim_ldap_filter},
		{"ldap-attrs",
		 &simplescim_ldap_attrs},
		{"ldap-attrsonly",
		 &simplescim_ldap_attrsonly}
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

/**
 * Initialises LDAP session.
 */
static int simplescim_ldap_init()
{
	int ldap_version = LDAP_VERSION3;
	struct berval cred;
	int err;

	/* Get configuration file variables related to LDAP */

	err = simplescim_ldap_get_variables();

	if (err == -1) {
		return -1;
	}

	/* Initialise LDAP session */

	err = ldap_initialize(&simplescim_ldap_ld, simplescim_ldap_uri);

	if (err != LDAP_SUCCESS) {
		simplescim_ldap_print_error(
			err,
			"ldap_initialize"
		);
		return -1;
	}

	/* Set protocol version */

	err = ldap_set_option(
		simplescim_ldap_ld,
		LDAP_OPT_PROTOCOL_VERSION,
		&ldap_version
	);

	if (err != LDAP_OPT_SUCCESS) {
		simplescim_ldap_print_error(
			err,
			"ldap_set_option"
		);
		simplescim_ldap_close();
		return -1;
	}

	/* Perform bind */

	cred.bv_val = (char *)simplescim_ldap_passwd;
	cred.bv_len = strlen(simplescim_ldap_passwd);

	err = ldap_sasl_bind_s(
		simplescim_ldap_ld,
		simplescim_ldap_who,
		LDAP_SASL_SIMPLE,
		&cred,
		NULL,
		NULL,
		NULL
	);

	if (err != LDAP_SUCCESS) {
		simplescim_ldap_print_error(
			err,
			"ldap_sasl_bind_s"
		);
		simplescim_ldap_close();
		return -1;
	}

	return 0;
}

/**
 * Performs the LDAP search operation.
 */
static int simplescim_ldap_search()
{
	int scope_val;
	const char *filter_val;
	char **attrs_val;
	int attrsonly_val;
	size_t i;
	int err;

	/* Set search scope */

	if (strcmp(simplescim_ldap_scope, "BASE") == 0) {
		scope_val = LDAP_SCOPE_BASE;
	} else if (strcmp(simplescim_ldap_scope, "ONELEVEL") == 0) {
		scope_val = LDAP_SCOPE_ONELEVEL;
	} else if (strcmp(simplescim_ldap_scope, "SUBTREE") == 0) {
		scope_val = LDAP_SCOPE_SUBTREE;
	} else if (strcmp(simplescim_ldap_scope, "CHILDREN") == 0) {
		scope_val = LDAP_SCOPE_CHILDREN;
	} else {
		simplescim_error_string_set_prefix(
			"simplescim_ldap_search"
		);
		simplescim_error_string_set_message(
"variable \"ldap-scope\" has invalid value \"%s\"\n"
"variable \"ldap-scope\" must have one of the following values:\n"
" BASE ONELEVEL SUBTREE CHILDREN",
			simplescim_ldap_scope
		);
		return -1;
	}

	/* Set filter */

	if (simplescim_ldap_filter == NULL
	    || simplescim_ldap_filter[0] == '\0') {
		filter_val = NULL;
	} else {
		filter_val = simplescim_ldap_filter;
	}

	/* Parse attrs */

	err = simplescim_ldap_attrs_parser(
		simplescim_ldap_attrs,
		&attrs_val
	);

	if (err == -1) {
		return -1;
	}

	/* Set attrsonly */

	if (strcmp(simplescim_ldap_attrsonly, "TRUE") == 0) {
		attrsonly_val = 1;
	} else if (strcmp(simplescim_ldap_attrsonly, "FALSE") == 0) {
		attrsonly_val = 0;
	} else {
		simplescim_error_string_set_prefix(
			"simplescim_ldap_search"
		);
		simplescim_error_string_set_message(
"variable \"ldap-attrsonly\" has invalid value \"%s\"\n"
"variable \"ldap-attrsonly\" must have one of the following values:\n"
" TRUE FALSE",
			simplescim_ldap_attrsonly
		);
		return -1;
	}

	/* Search */

	err = ldap_search_ext_s(
		simplescim_ldap_ld,
		simplescim_ldap_base,
		scope_val,
		filter_val,
		attrs_val,
		attrsonly_val,
		NULL,
		NULL,
		NULL,
		-1,
		&simplescim_ldap_res
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
 * Clones a NULL-terminated list of pointers to
 * struct berval values into a dynamically allocated
 * struct simplescim_arbval_list object.
 * On success, a pointer to the cloned object is returned.
 * On error, NULL is returned and simplescim_error_string
 * is set to an appropriate error message.
 */
static struct simplescim_arbval_list *clone_vals(
	const struct berval **vals
)
{
	struct simplescim_arbval_list *clone;
	struct simplescim_arbval *val;
	size_t len;
	size_t i;
	int err;

	if (vals == NULL) {
		simplescim_error_string_set(
			"clone_vals",
			"cannot clone NULL"
		);
		return NULL;
	}

	/* Calculate the length of vals */

	len = 0;

	for (i = 0; vals[i] != NULL; ++i) {
		++len;
	}

	/* Allocate the clone */

	clone = simplescim_arbval_list_new(len);

	if (clone == NULL) {
		return NULL;
	}

	/* Clone and insert all values */

	for (i = 0; i < len; ++i) {
		/* Clone value i */

		val = simplescim_arbval_new(
			vals[i]->bv_len,
			(const uint8_t *)vals[i]->bv_val
		);

		if (val == NULL) {
			simplescim_arbval_list_delete(clone);
			return NULL;
		}

		/* Insert the value */
		err = simplescim_arbval_list_append(
			clone,
			val
		);

		if (err == -1) {
			simplescim_arbval_delete(val);
			simplescim_arbval_list_delete(clone);
			return NULL;
		}
	}

	return clone;
}

/**
 * Converts an entry in the LDAP search results into a user.
 * On success, a pointer to a new user object is returned.
 * On error, NULL is returned and simplescim_error_string
 * is set to an appropriate error message.
 */
static struct simplescim_user *entry_to_user(LDAPMessage *entry)
{
	struct simplescim_user *user;
	char *attr, *attr_clone;
	struct berval **vals;
	struct simplescim_arbval_list *vals_clone;
	BerElement *ber;
	int ld_errno;
	int err;

	/* Create the user object. */

	user = simplescim_user_new();

	for (attr = ldap_first_attribute(simplescim_ldap_ld, entry, &ber);
	     attr != NULL;
	     attr = ldap_next_attribute(simplescim_ldap_ld, entry, ber)) {
		/* Clone 'attr'. */
		attr_clone = strdup(attr);

		if (attr_clone == NULL) {
			simplescim_error_string_set_errno(
				"entry_to_user:"
				"strdup"
			);
			ldap_memfree(attr);
			ber_free(ber, 0);
			simplescim_user_delete(user);
			return NULL;
		}

		/* Get and clone 'vals'. */
		vals = ldap_get_values_len(
			simplescim_ldap_ld,
			entry,
			attr
		);

		if (vals == NULL) {
			err = ldap_get_option(
				simplescim_ldap_ld,
				LDAP_OPT_RESULT_CODE,
				&ld_errno
			);

			if (err != LDAP_OPT_SUCCESS) {
				simplescim_ldap_print_error(
					err,
					"ldap_get_option"
				);
			} else {
				simplescim_ldap_print_error(
					ld_errno,
					"ldap_get_values_len"
				);
			}

			free(attr_clone);
			ldap_memfree(attr);
			ber_free(ber, 0);
			simplescim_user_delete(user);
			return NULL;
		}

		vals_clone = clone_vals((const struct berval **)vals);
 
		if (vals_clone == NULL) {
			ldap_value_free_len(vals);
			free(attr_clone);
			ldap_memfree(attr);
			ber_free(ber, 0);
			simplescim_user_delete(user);
			return NULL;
		}

		/* Set attribute in user object */

		err = simplescim_user_set_attribute(
			user,
			attr_clone,
			vals_clone
		);

		if (err == -1) {
			simplescim_arbval_list_delete(vals_clone);
			ldap_value_free_len(vals);
			free(attr_clone);
			ldap_memfree(attr);
			ber_free(ber, 0);
			simplescim_user_delete(user);
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
 * Construct the user list object from the LDAP response.
 * On success, a pointer to the constructed object is
 * returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate
 * error message.
 */
static struct simplescim_user_list *simplescim_ldap_to_user_list()
{
	struct simplescim_user_list *users;
	struct simplescim_user *user;
	struct simplescim_arbval *uid;
	LDAPMessage *entry;
	int err;

	/* Initialise user list */

	users = simplescim_user_list_new();

	/* For every response entry, create a user and
	   insert it into the user list. */

	for (entry = ldap_first_entry(simplescim_ldap_ld,
	                              simplescim_ldap_res);
	     entry != NULL;
	     entry = ldap_next_entry(simplescim_ldap_ld, entry)) {

		/* Create the user. */

		user = entry_to_user(entry);

		if (user == NULL) {
			simplescim_user_list_delete(users);
			return NULL;
		}

		/* Get the user's unique identifier. */

		uid = simplescim_user_get_uid(user);

		if (uid == NULL) {
			simplescim_user_delete(user);
			simplescim_user_list_delete(users);
			return NULL;
		}

		/* Insert user into user list. */

		err = simplescim_user_list_insert_user(
			users,
			uid,
			user
		);

		if (err == -1) {
			simplescim_arbval_delete(uid);
			simplescim_user_delete(user);
			simplescim_user_list_delete(users);
			return NULL;
		}
	}

	return users;
}

/**
 * Reads user data from LDAP into a user list object
 * according to the configuration file and returns a
 * pointer it. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
struct simplescim_user_list *simplescim_ldap_get_users()
{
	struct simplescim_user_list *users;
	int err;

	/* Initialise LDAP session. */

	err = simplescim_ldap_init();

	if (err == -1) {
		return NULL;
	}

	/* Perform search operation. */

	err = simplescim_ldap_search();

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
