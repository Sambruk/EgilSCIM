#include "simplescim_user.h"

#include <stdlib.h>
#include <string.h>
#include "uthash.h"

#include "simplescim_error_string.h"
#include "simplescim_arbval.h"
#include "simplescim_arbval_list.h"
#include "simplescim_config_file.h"

struct attribute_record {
	char *attribute;
	struct simplescim_arbval_list *values;
	UT_hash_handle hh;
};

struct simplescim_user {
	size_t n_attributes;
	struct attribute_record *attributes;
};

/**
 * Creates a new simplescim_user object.
 * On success, a pointer to the new object is returned.
 * On error, NULL is returned and simplescim_error_string
 * is set to an appropriate error message.
 */
struct simplescim_user *simplescim_user_new()
{
	struct simplescim_user *this;

	this = malloc(sizeof(struct simplescim_user));

	if (this == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_user_new:"
			"malloc"
		);
		return NULL;
	}

	this->n_attributes = 0;
	this->attributes = NULL;

	return this;
}

/**
 * Deletes 'this' and all associated dynamically allocated
 * memory.
 */
void simplescim_user_delete(
	struct simplescim_user *this
)
{
	struct attribute_record *s, *tmp;

	if (this != NULL) {
		if (this->attributes != NULL) {
			HASH_ITER(hh, this->attributes, s, tmp) {
				HASH_DEL(this->attributes, s);
				free(s->attribute);
				simplescim_arbval_list_delete(s->values);
				free(s);
			}
		}

		free(this);
	}
}

/**
 * Returns the number of attributes in 'this'.
 */
size_t simplescim_user_get_n_attributes(
	const struct simplescim_user *this
)
{
	return this->n_attributes;
}

/**
 * Returns a copy of 'this'.
 * On success, a pointer to a newly allocated user is
 * returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
struct simplescim_user *simplescim_user_copy(
	const struct simplescim_user *this
)
{
	struct simplescim_user *copy;
	struct attribute_record *s, *tmp;
	char *attribute;
	struct simplescim_arbval_list *values;
	int err;

	/* Allocate copy */
	copy = simplescim_user_new();

	if (copy == NULL) {
		return NULL;
	}

	/* Copy every attribute */
	HASH_ITER(hh, this->attributes, s, tmp) {
		/* Copy attribute name */
		attribute = strdup(s->attribute);

		if (attribute == NULL) {
			simplescim_error_string_set_errno(
				"simplescim_user_copy:"
				"strdup"
			);
			simplescim_user_delete(copy);
			return NULL;
		}

		/* Copy values */
		values = simplescim_arbval_list_copy(
			s->values
		);

		if (values == NULL) {
			free(attribute);
			simplescim_user_delete(copy);
			return NULL;
		}

		/* Insert copied attribute name and values
		   into copied user */
		err = simplescim_user_set_attribute(
			copy,
			attribute,
			values
		);

		if (err == -1) {
			simplescim_arbval_list_delete(values);
			free(attribute);
			simplescim_user_delete(copy);
			return NULL;
		}
	}

	return copy;
}

/**
 * Returns the unique identifier of 'this'.
 * On success, a pointer to a newly allocated struct
 * simplescim_arbval object is returned. On error, NULL is
 * returned and simplescim_error_string is set to an
 * appropriate error message.
 */
struct simplescim_arbval *simplescim_user_get_uid(
	const struct simplescim_user *this
)
{
	struct simplescim_arbval *uid;
	const char *uid_attr;
	const struct simplescim_arbval_list *values;
	int err;

	/* Get LDAP attribute that is unique identifier */
	err = simplescim_config_file_require(
		"user-unique-identifier",
		&uid_attr
	);

	if (err == -1) {
		return NULL;
	}

	/* Get unique identifier value */
	err = simplescim_user_get_attribute(
		this,
		uid_attr,
		&values
	);

	if (err == -1) {
		simplescim_error_string_set_prefix(
			"simplescim_user_get_uid:"
			"simplescim_user_get_attribute"
		);
		simplescim_error_string_set_message(
			"user does not have attribute \"%s\"",
			uid_attr
		);
		return NULL;
	}

	if (values->al_len != 1) {
		simplescim_error_string_set_prefix(
			"simplescim_user_get_uid"
		);
		simplescim_error_string_set_message(
			"attribute \"%s\" must have exactly one value",
			uid_attr
		);
		return NULL;
	}

	/* Allocate unique identifier copy */
	uid = simplescim_arbval_copy(values->al_vals[0]);

	if (uid == NULL) {
		return NULL;
	}

	return uid;
}

/**
 * Associates 'attribute' with 'values' in 'this'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_user_set_attribute(
	struct simplescim_user *this,
	char *attribute,
	struct simplescim_arbval_list *values
)
{
	struct attribute_record *s;

	/* Check if 'attribute' is already in the hash
	   table, in which case 'values' should replace
	   previous values associated with 'attribute' in
	   the hash table rather than be inserted. */
	HASH_FIND_STR(this->attributes, attribute, s);

	if (s == NULL) {
		/* 'attribute' is not in the hash table so
		   'values' should be inserted. */
		s = malloc(sizeof(struct attribute_record));

		if (s == NULL) {
			simplescim_error_string_set_errno(
				"simplescim_user_set_attribute:"
				"malloc"
			);
			return -1;
		}

		s->attribute = attribute;
		s->values = values;

		HASH_ADD_KEYPTR(hh,
		                this->attributes,
		                s->attribute,
		                strlen(s->attribute),
		                s);

		++this->n_attributes;
	} else {
		/* 'attribute' is already in the hash table, so
		   'values' should replace previous values
		   associated with 'attribute'. 'attribute' can be
		   freed since an identical dynamically allocated
		   object is already in the hash table. */
		free(attribute);
		simplescim_arbval_list_delete(s->values);
		s->values = values;
	}

	return 0;
}

/**
 * Gets the values associated with 'attribute' in 'this'
 * and stores them in 'valuesp'.
 * If 'attribute' has associated values, zero is returned.
 * Otherwise, -1 is returned.
 */
int simplescim_user_get_attribute(
	const struct simplescim_user *this,
	const char *attribute,
	const struct simplescim_arbval_list **valuesp
)
{
	struct attribute_record *s;

	HASH_FIND_STR(this->attributes, attribute, s);

	if (s == NULL) {
		return -1;
	}

	if (valuesp != NULL) {
		*valuesp = s->values;
	}

	return 0;
}

/**
 * Performs 'func' for every attribute in 'this'.
 * 'func' must have the following definition:
 * int func(const char *attribute,
 *          const struct simplescim_arbval_list *values);
 */
int simplescim_user_foreach(
	const struct simplescim_user *this,
	int (*func)(const char *attribute,
	            const struct simplescim_arbval_list *values)
)
{
	struct attribute_record *s, *tmp;
	int err;

	HASH_ITER(hh, this->attributes, s, tmp) {
		err = func(s->attribute, s->values);

		if (err == -1) {
			return -1;
		}
	}

	return 0;
}

/**
 * Returns 1 if 'this' ⊆ 'other'.
 * Returns 0 otherwise.
 */
int simplescim_user_subset_eq(
	const struct simplescim_user *this,
	const struct simplescim_user *other
)
{
	const char *user_scim_resource_identifier;
	struct attribute_record *s, *tmp;
	const struct simplescim_arbval_list *this_vals;
	const struct simplescim_arbval_list *other_vals;
	int err;

	err = simplescim_config_file_require(
		"user-scim-resource-identifier",
		&user_scim_resource_identifier
	);

	if (err == -1) {
		/* return -1; ? */
		return 0;
	}

	/* For every attribute in 'this' */
	HASH_ITER(hh, this->attributes, s, tmp) {
		if (strcmp(s->attribute,
		           user_scim_resource_identifier) == 0) {
			continue;
		}

		/* Get the values associated with the
		   attribute in 'this' */
		this_vals = s->values;

		/* Get the values associated with the
		   attribute in 'other' */
		err = simplescim_user_get_attribute(
			other,
			s->attribute,
			&other_vals
		);

		/* Verify that the attribute exists in 'other' */
		if (err == -1) {
			return 0;
		}

		/* Verify that this_vals ⊆ other_vals */
		if (!simplescim_arbval_list_subset_eq(this_vals,
		                                      other_vals)) {
			return 0;
		}
	}

	return 1;
}

/**
 * Returns 1 if 'this' = 'other'.
 * Returns 0 otherwise.
 */
int simplescim_user_eq(
	const struct simplescim_user *this,
	const struct simplescim_user *other
)
{
	return simplescim_user_subset_eq(this, other)
	       && simplescim_user_subset_eq(other, this);
}
