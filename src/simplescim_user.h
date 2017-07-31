#ifndef SIMPLESCIM_USER_H
#define SIMPLESCIM_USER_H

#include <lber.h>

struct simplescim_user;

/**
 * Creates a new simplescim_user object.
 * On success, a pointer to the new object is returned.
 * On error, NULL is returned and simplescim_error_string
 * is set to an appropriate error message.
 */
struct simplescim_user *simplescim_user_new();

/**
 * Deletes 'this' and all associated dynamically allocated
 * memory.
 */
void simplescim_user_delete(struct simplescim_user *this);

/**
 * Returns the number of attributes in 'this'.
 */
size_t simplescim_user_get_n_attributes(
	const struct simplescim_user *this
);

/**
 * Returns a copy of 'this'.
 * On success, a pointer to a newly allocated user is
 * returned. On error, NULL is returned and
 * simplescim_error_string is set tot an appropriate error
 * message.
 */
struct simplescim_user *simplescim_user_copy(
	const struct simplescim_user *this
);

/**
 * Returns the unique identifier of 'this'.
 * On success, a pointer to a newly allocated struct berval
 * is returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
struct berval *simplescim_user_get_uid(
	const struct simplescim_user *this
);

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
                                  struct berval **values);

/**
 * Gets the values associated with 'attribute' in 'this' as
 * a NULL-terminated array of struct berval pointers and
 * stores them in 'valuesp'.
 * If 'attribute' has associated values, zero is returned.
 * Otherwise, -1 is returned.
 */
int simplescim_user_get_attribute(const struct simplescim_user *this,
                                  const char *attribute,
                                  const struct berval ***valuesp);

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
);

/**
 * Returns 1 if 'this' ⊆ 'other'.
 * Returns 0 otherwise.
 */
int simplescim_user_subset_eq(const struct simplescim_user *this,
                              const struct simplescim_user *other);

/**
 * Returns 1 if 'this' = 'other'.
 * Returns 0 otherwise.
 */
int simplescim_user_eq(const struct simplescim_user *this,
                       const struct simplescim_user *other);

#endif
