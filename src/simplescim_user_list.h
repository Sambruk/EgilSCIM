#ifndef SIMPLESCIM_USER_LIST_H
#define SIMPLESCIM_USER_LIST_H

#include <lber.h>

#include "simplescim_user.h"

struct simplescim_user_list;

/**
 * Creates a new simplescim_user_list object.
 * On success, a pointer to the new object is returned.
 * On error, NULL is returned and simplescim_error_string
 * is set to an appropriate error message.
 */
struct simplescim_user_list *simplescim_user_list_new();

/**
 * Deletes 'this' and all associated dynamically allocated
 * memory.
 */
void simplescim_user_list_delete(struct simplescim_user_list *this);

/**
 * Returns the number of users in 'this'.
 */
size_t simplescim_user_list_get_n_users(
	const struct simplescim_user_list *this
);

/**
 * Associates 'unique_identifier' with 'user' in 'this'.
 * 'unique_identifier' is a dynamically allocated
 * struct berval object and 'user' is a dynamically
 * allocated simplescim_user object.
 * On success, zero is returned. On error, -1 is
 * returned and simplescim_error_string is set to an
 * appropriate error message.
 */
int simplescim_user_list_insert_user(struct simplescim_user_list *this,
                                     struct berval *unique_identifier,
                                     struct simplescim_user *user);

/**
 * Gets the user associated with 'unique_identifier' in
 * 'this' and stores it in 'userp'.
 * If 'unique_identifier' has an associated user, zero is
 * returned. Otherwise, -1 is returned.
 */
int simplescim_user_list_get_user(const struct simplescim_user_list *this,
                                  const struct berval *unique_identifier,
                                  const struct simplescim_user **userp);

/**
 * Performs 'func' for every user in 'this'.
 * 'func' must have the following definition:
 * void func(const struct berval *unique_identifier,
 *           const struct simplescim_user *user);
 */
void simplescim_user_list_foreach(
	const struct simplescim_user_list *this,
	void (*func)(const struct berval *unique_identifier,
	             const struct simplescim_user *user)
);

/**
 * Compares 'this' to 'cache' and performs
 * 'create_user_func' on users in 'this' but not in
 * 'cache', performs 'update_user_func' on users in both
 * 'this' and 'cache' if the user has been updated and
 * performs 'delete_user_func' on users in 'cache' but not
 * in 'this'.
 */
int simplescim_user_list_find_changes(
	const struct simplescim_user_list *this,
	const struct simplescim_user_list *cache,
	int (create_user_func)(const struct simplescim_user *user),
	int (update_user_func)(const struct simplescim_user *user,
	                       const struct simplescim_user *cached_user),
	int (delete_user_func)(const struct simplescim_user *cached_user)
);

#endif
