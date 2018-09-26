/**
 * Copyright © 2017-2018  Max Wällstedt <>
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

#ifndef SIMPLESCIM_USER_H
#define SIMPLESCIM_USER_H

#include <stddef.h>

#include "simplescim_arbval.h"
#include "simplescim_arbval_list.h"

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
void simplescim_user_delete(
	struct simplescim_user *this
);

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
 * simplescim_error_string is set to an appropriate error
 * message.
 */
struct simplescim_user *simplescim_user_copy(
	const struct simplescim_user *this
);

/**
 * Returns the unique identifier of 'this'.
 * On success, a pointer to a newly allocated struct
 * simplescim_arbval object is returned. On error, NULL is
 * returned and simplescim_error_string is set to an
 * appropriate error message.
 */
struct simplescim_arbval *simplescim_user_get_uid(
	const struct simplescim_user *this
);

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
);

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
);

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
);

/**
 * Returns 1 if 'this' ⊆ 'other'.
 * Returns 0 otherwise.
 */
int simplescim_user_subset_eq(
	const struct simplescim_user *this,
	const struct simplescim_user *other
);

/**
 * Returns 1 if 'this' = 'other'.
 * Returns 0 otherwise.
 */
int simplescim_user_eq(
	const struct simplescim_user *this,
	const struct simplescim_user *other
);

#endif
