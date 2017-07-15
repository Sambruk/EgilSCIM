#include "simplescim_user.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <lber.h>
#include "uthash.h"

#include "simplescim_error_string.h"
#include "simplescim_config_file.h"

/**
 * The uthash record for the user object.
 */
struct attribute_record {
	char *attribute;
	struct berval **values;
	UT_hash_handle hh;
};

/**
 * The user object. It is only a wrapper around the above
 * uthash record.
 */
struct simplescim_user {
	struct attribute_record *attributes;
};

/**
 * Creates a new user object.
 * On success, a pointer to the new object is returned. On
 * error, NULL is returned and 'simplescim_error_string' is
 * set to an appropriate error message.
 */
struct simplescim_user *simplescim_user_new()
{
	struct simplescim_user *this;

	this = malloc(sizeof(struct simplescim_user));

	if (this == NULL) {
		sprintf(simplescim_error_string,
		        "%s:simplescim_user_new: %s",
		        simplescim_config_file_name,
		        strerror(errno));
		return NULL;
	}

	this->attributes = NULL;

	return this;
}

/**
 * Deletes a dynamically allocated NULL-terminated list of
 * pointers to 'struct berval' variables.
 */
static void simplescim_user_delete_values(struct berval **values)
{
	size_t i;

	if (values == NULL) {
		return;
	}

	for (i = 0; values[i] != NULL; ++i) {
		free(values[i]->bv_val);
		free(values[i]);
	}

	free(values);
}

/**
 * Deletes a user object and all dynamically allocated
 * memory associated with it.
 */
void simplescim_user_delete(struct simplescim_user *this)
{
	struct attribute_record *s, *tmp;

	HASH_ITER(hh, this->attributes, s, tmp) {
		HASH_DEL(this->attributes, s);
		free(s->attribute);
		simplescim_user_delete_values(s->values);
		free(s);
	}

	free(this);
}

/**
 * Associates 'values' with 'attribute'. 'attribute' is a
 * dynamically allocated null-terminated string and
 * 'values' is a dynamically allocated NULL-terminated list
 * of pointers to 'struct berval' variables.
 * On success, zero is returned. On error, -1 is
 * returned and 'simplescim_error_string' is set to an
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
			sprintf(simplescim_error_string,
			        "%s:simplescim_user_set_attribute: %s",
			        simplescim_config_file_name,
			        strerror(errno));
			return -1;
		}

		s->attribute = attribute;
		s->values = values;

		HASH_ADD_KEYPTR(hh,
		                this->attributes,
		                s->attribute,
		                strlen(s->attribute),
		                s);

		return 0;
	}

	/* 'attribute' is already in the hash table, so
	   'values' should replace previous values
	   associated with 'attribute'. 'attribute' can be
	   freed since an identical dynamically allocated
	   object is already in the hash table. */
	free(attribute);
	simplescim_user_delete_values(s->values);
	s->values = values;

	return 0;
}

/**
 * Gets the values associated with 'attribute' and stores
 * them in 'valuesp'. 'valuesp' is a pointer to a
 * NULL-terminated list of pointers to 'struct berval'
 * variables.
 * If 'attribute' has associated values, zero is returned.
 * Otherwise, -1 is returned and 'valuesp' remains
 * untouched.
 */
int simplescim_user_get_attribute(struct simplescim_user *this,
                                  const char *attribute,
                                  const struct berval ***valuesp)
{
	struct attribute_record *s;

	HASH_FIND_STR(this->attributes, attribute, s);

	if (s == NULL) {
		return -1;
	}

	*valuesp = (const struct berval **)s->values;

	return 0;
}

/**
 * Perform 'func' for every attribute in the user.
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
