#ifndef SIMPLESCIM_USER_LIST_H
#define SIMPLESCIM_USER_LIST_H

#include "simplescim_user.h"

struct simplescim_user_list;

/**
 * Creates a new user list object.
 * On success, a pointer to the new object is returned. On
 * error, NULL is returned and 'simplescim_error_string' is
 * set to an appropriate error message.
 */
struct simplescim_user_list *simplescim_user_list_new();

/**
 * Deletes a user list object and all dynamically allocated
 * memory associated with it.
 */
void simplescim_user_list_delete(struct simplescim_user_list *this);

/**
 * Associates 'user' with 'unique_identifier'.
 * 'unique_identifier' is a dynamically allocated
 * 'struct berval' and 'user' is a dynamically allocated
 * user object.
 * On success, zero is returned. On error, -1 is
 * returned and 'simplescim_error_string' is set to an
 * appropriate error message.
 */
int simplescim_user_list_insert_user(struct simplescim_user_list *this,
                                     struct berval *unique_identifier,
                                     struct simplescim_user *user);

/**
 * Gets the user associated with 'unique_identifier' and
 * stores it in 'userp'. 'userp' is a pointer to a user
 * object.
 * If 'unique_identifier' has an associated user, zero is
 * returned. Otherwise, -1 is returned and 'userp' remains
 * untouched.
 */
int
simplescim_user_list_get_user(struct simplescim_user_list *this,
                              const struct berval *unique_identifier,
                              const struct simplescim_user **userp);

/**
 * Perform 'func' for every user in the user list.
 * 'func' must have the following definition:
 * void func(const struct berval *unique_identifier,
 *           const struct simplescim_user *user);
 */
void simplescim_user_list_foreach(
	struct simplescim_user_list *this,
	void (*func)(const struct berval *unique_identifier,
	             const struct simplescim_user *user)
);

#endif
