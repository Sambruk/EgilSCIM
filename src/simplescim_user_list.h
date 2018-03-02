/**
 * Copyright © 2017-2018  Max Wällstedt <max.wallstedt@gmail.com>
 *
 * This file is part of SimpleSCIM.
 *
 * SimpleSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with SimpleSCIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SIMPLESCIM_USER_LIST_H
#define SIMPLESCIM_USER_LIST_H

#include <stddef.h>

#include "simplescim_arbval.h"
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
void simplescim_user_list_delete(
	struct simplescim_user_list *this
);

/**
 * Returns the number of users in 'this'.
 */
size_t simplescim_user_list_get_n_users(
	const struct simplescim_user_list *this
);

/**
 * Associates 'uid' with 'user' in 'this'.
 * On success, zero is returned. On error, -1 is
 * returned and simplescim_error_string is set to an
 * appropriate error message.
 */
int simplescim_user_list_insert_user(
	struct simplescim_user_list *this,
	struct simplescim_arbval *uid,
	struct simplescim_user *user
);

/**
 * Gets the user associated with 'uid' in 'this' and stores
 * it in 'userp'.
 * If 'uid' has an associated user, zero is returned.
 * Otherwise, -1 is returned.
 */
int simplescim_user_list_get_user(
	const struct simplescim_user_list *this,
	const struct simplescim_arbval *uid,
	const struct simplescim_user **userp
);

/**
 * Performs 'func' for every user in 'this'.
 * 'func' must have the following definition:
 * int func(const struct simplescim_arbval *uid,
 *          const struct simplescim_user *user);
 */
int simplescim_user_list_foreach(
	const struct simplescim_user_list *this,
	int (*func)(const struct simplescim_arbval *uid,
	            const struct simplescim_user *user)
);

/**
 * Compares 'this' to 'cache' and performs 'copy_user_func'
 * on users in both 'this' and cache if they are equal,
 * 'create_user_func' on users in 'this' but not in
 * 'cache', performs 'update_user_func' on users in both
 * 'this' and 'cache' if the user has been updated and
 * performs 'delete_user_func' on users in 'cache' but not
 * in 'this'.
 */
int simplescim_user_list_find_changes(
	const struct simplescim_user_list *this,
	const struct simplescim_user_list *cache,
	int (copy_user_func)(const struct simplescim_user *cached_user),
	int (create_user_func)(const struct simplescim_user *user),
	int (update_user_func)(const struct simplescim_user *user,
	                       const struct simplescim_user *cached_user),
	int (delete_user_func)(const struct simplescim_user *cached_user)
);

#endif
