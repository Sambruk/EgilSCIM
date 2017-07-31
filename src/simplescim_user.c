#include "simplescim_user.h"

#include <stdlib.h>
#include <string.h>
#include <lber.h>
#include "uthash.h"

#include "simplescim_error_string.h"
#include "simplescim_config_file.h"

struct attribute_record {
	char *attribute;
	struct berval **values;
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
			"simplescim_user_new:malloc"
		);
		return NULL;
	}

	this->n_attributes = 0;
	this->attributes = NULL;

	return this;
}

/**
 * Deletes 'values', which is a dynamically allocated
 * NULL-terminated array of struct berval pointers.
 */
static void delete_values(struct berval **values)
{
	size_t i;

	if (values != NULL) {
		for (i = 0; values[i] != NULL; ++i) {
			free(values[i]->bv_val);
			free(values[i]);
		}

		free(values);
	}
}

/**
 * Deletes 'this' and all associated dynamically allocated
 * memory.
 */
void simplescim_user_delete(struct simplescim_user *this)
{
	struct attribute_record *s, *tmp;

	HASH_ITER(hh, this->attributes, s, tmp) {
		HASH_DEL(this->attributes, s);
		free(s->attribute);
		delete_values(s->values);
		free(s);
	}

	free(this);
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

static struct berval *simplescim_user_copy_value(
	const struct berval *value
)
{
	struct berval *val;

	val = malloc(sizeof(struct berval));

	if (val == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_user_copy_value:"
			"malloc"
		);
		return NULL;
	}

	val->bv_len = value->bv_len;
	val->bv_val = malloc(val->bv_len + 1);

	if (val->bv_val == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_user_copy_value:"
			"malloc"
		);
		free(val);
		return NULL;
	}

	memcpy(val->bv_val, value->bv_val, val->bv_len);
	val->bv_val[val->bv_len] = '\0';

	return val;
}

static void simplescim_user_delete_value(
	struct berval *value
)
{
	if (value != NULL) {
		if (value->bv_val != NULL) {
			free(value->bv_val);
		}

		free(value);
	}
}

static struct berval **simplescim_user_copy_values(
	const struct berval **values
)
{
	struct berval **vals;
	size_t len;
	size_t i;

	/* Determine the number of values */
	len = 0;

	while (values[len] != NULL) {
		++len;
	}

	/* Allocate copied values */
	vals = malloc(sizeof(struct berval *) * (len + 1));

	if (vals == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_user_copy_values:"
			"malloc"
		);
		return NULL;
	}

	/* Copy every value */
	for (i = 0; i < len; ++i) {
		vals[i] = simplescim_user_copy_value(values[i]);

		if (vals[i] == NULL) {
			while (i > 0) {
				simplescim_user_delete_value(vals[i - 1]);
				--i;
			}
			free(vals);
			return NULL;
		}
	}

	vals[len] = NULL;

	return vals;
}

static void simplescim_user_delete_values(
	struct berval **values
)
{
	size_t i;

	for (i = 0; values[i] != NULL; ++i) {
		simplescim_user_delete_value(values[i]);
	}

	free(values);
}

/**
 * Returns a copy of 'this'.
 * On success, a pointer to a newly allocated user is
 * returned. On error, NULL is returned and
 * simplescim_error_string is set tot an appropriate error
 * message.
 */
struct simplescim_user *simplescim_user_copy(
	const struct simplescim_user *this
)
{
	struct simplescim_user *copy;
	struct attribute_record *s, *tmp;
	char *attribute;
	struct berval **values;
	int err;

	copy = simplescim_user_new();

	if (copy == NULL) {
		return NULL;
	}

	HASH_ITER(hh, this->attributes, s, tmp) {
		attribute = strdup(s->attribute);

		if (attribute == NULL) {
			simplescim_error_string_set_errno(
				"simplescim_user_copy:"
				"strdup"
			);
			simplescim_user_delete(copy);
			return NULL;
		}

		values = simplescim_user_copy_values(
			(const struct berval **)s->values
		);

		if (values == NULL) {
			free(attribute);
			simplescim_user_delete(copy);
			return NULL;
		}

		err = simplescim_user_set_attribute(
			copy,
			attribute,
			values
		);

		if (err == -1) {
			simplescim_user_delete_values(values);
			free(attribute);
			simplescim_user_delete(copy);
			return NULL;
		}
	}

	return copy;
}

/**
 * Returns the unique identifier of 'this'.
 * On success, a pointer to a newly allocated struct berval
 * is returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
struct berval *simplescim_user_get_uid(
	const struct simplescim_user *this
)
{
	struct berval *uid;
	const char *uid_attr;
	const struct berval **values;
	int err;

	/* Get LDAP attribute that is unique identifier */
	err = simplescim_config_file_get(
		"ldap-unique-identifier",
		&uid_attr
	);

	if (err == -1) {
		simplescim_error_string_set(
			"simplescim_user_get_uid",
			"'ldap-unique-identifier' not set"
		);
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
			"simplescim_user_get_uid"
		);
		simplescim_error_string_set_message(
			"user does not have attribute \"%s\"",
			uid_attr
		);
		return NULL;
	}

	if (values[0] == NULL) {
		simplescim_error_string_set_prefix(
			"simplescim_user_get_uid"
		);
		simplescim_error_string_set_message(
			"attribute \"%s\" is empty",
			uid_attr
		);
		return NULL;
	}

	/* Allocate unique identifier copy */
	uid = simplescim_user_copy_value(values[0]);

	if (uid == NULL) {
		return NULL;
	}

	return uid;
}

/**
 * Associates 'attribute' with 'values' in 'this'.
 * 'attribute' is a dynamically allocated null-terminated
 * string and 'values' is a dynamically allocated
 * NULL-terminated array of struct berval pointers.
 * On success, zero is returned. On error, -1 is
 * returned and simplescim_error_string is set to an
 * appropriate error message.
 */
int simplescim_user_set_attribute(struct simplescim_user *this,
                                  char *attribute,
                                  struct berval **values)
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
				"simplescim_user_set_attribute:malloc"
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
		delete_values(s->values);
		s->values = values;
	}

	return 0;
}

/**
 * Gets the values associated with 'attribute' in 'this' as
 * a NULL-terminated array of struct berval pointers and
 * stores them in 'valuesp'.
 * If 'attribute' has associated values, zero is returned.
 * Otherwise, -1 is returned.
 */
int simplescim_user_get_attribute(const struct simplescim_user *this,
                                  const char *attribute,
                                  const struct berval ***valuesp)
{
	struct attribute_record *s;

	HASH_FIND_STR(this->attributes, attribute, s);

	if (s == NULL) {
		return -1;
	}

	if (valuesp != NULL) {
		*valuesp = (const struct berval **)s->values;
	}

	return 0;
}

/**
 * Performs 'func' for every attribute in 'this'.
 * 'func' must have the following definition:
 * void func(const char *attribute,
 *           const struct berval **values);
 */
void simplescim_user_foreach(
	const struct simplescim_user *this,
	void (*func)(const char *attribute,
	             const struct berval **values)
)
{
	struct attribute_record *s, *tmp;

	HASH_ITER(hh, this->attributes, s, tmp) {
		func(s->attribute, (const struct berval **)s->values);
	}
}

/**
 * Returns 1 if 'this' ⊆ 'other'.
 * Returns 0 otherwise.
 */
int simplescim_user_subset_eq(const struct simplescim_user *this,
                              const struct simplescim_user *other)
{
	struct attribute_record *s, *tmp;
	const struct berval **this_vals, **other_vals;
	size_t i, j, k;
	int err;

	/* For every attribute in 'this' */
	HASH_ITER(hh, this->attributes, s, tmp) {
		/* Get the values associated with the
		   attribute in 'this' */
		this_vals = (const struct berval **)s->values;

		/* Get the values associated with the
		   attribute in 'other' */
		err = simplescim_user_get_attribute(other,
		                                    s->attribute,
		                                    &other_vals);

		/* Verify that the attribute exists in 'other' */
		if (err == -1) {
			return 0;
		}

		/* If both values are NULL, they're equal */
		if (this_vals == NULL && other_vals == NULL) {
			continue;
		}

		/* If one value is NULL, they're not equal */
		if (this_vals == NULL || other_vals == NULL) {
			return 0;
		}

		/* For every value of the attribute in 'this',
		   find an equal value of the attribute in 'other' */
		for (i = 0; this_vals[i] != NULL; ++i) {
			for (j = 0; other_vals[j] != NULL; ++j) {
				/* If values have different lengths,
				   it is not the match */
				if (this_vals[i]->bv_len
				    != other_vals[j]->bv_len) {
					continue;
				}

				/* Verify that both values have
				   equal contents */
				for (k = 0;
				     k < this_vals[i]->bv_len;
				     ++k) {
					/* If one byte is different,
					   they're not equal */
					if (this_vals[i]->bv_val[k]
					    != other_vals[j]->bv_val[k]) {
						break;
					}
				}

				/* If the above loop was terminated
				   early, it is not a match. */
				if (k < this_vals[i]->bv_len) {
					continue;
				}

				/* Match was found, terminate loop */
				break;
			}

			/* If above loop was not terminated early,
			   no match was found */
			if (other_vals[j] == NULL) {
				return 0;
			}
		}
	}

	return 1;
}

/**
 * Returns 1 if 'this' = 'other'.
 * Returns 0 otherwise.
 */
int simplescim_user_eq(const struct simplescim_user *this,
                       const struct simplescim_user *other)
{
	return simplescim_user_subset_eq(this, other)
	       && simplescim_user_subset_eq(other, this);
}
