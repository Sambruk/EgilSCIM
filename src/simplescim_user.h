#ifndef SIMPLESCIM_USER_H
#define SIMPLESCIM_USER_H

#include <lber.h>

struct simplescim_user;

/**
 * Creates a new user object.
 * On success, a pointer to the new object is returned. On
 * error, NULL is returned and 'simplescim_error_string' is
 * set to an appropriate error message.
 */
struct simplescim_user *simplescim_user_new();

/**
 * Deletes a user object and all dynamically allocated
 * memory associated with it.
 */
void simplescim_user_delete(struct simplescim_user *this);

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
                                  struct berval **values);

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
                                  const struct berval ***valuesp);

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
);

#endif
