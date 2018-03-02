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

#include "simplescim_arbval_list.h"

#include <stdlib.h>
#include <stdint.h>

#include "simplescim_error_string.h"
#include "simplescim_arbval.h"

/**
 * Allocates space for at least 'len' struct
 * simplescim_arbval pointers in 'al'. 'al' must be
 * allocated.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_arbval_list_constructor(
	struct simplescim_arbval_list *al,
	uint64_t len
)
{
	uint64_t al_alloc;
	struct simplescim_arbval **al_vals;

	if (al == NULL) {
		simplescim_error_string_set(
			"simplescim_arbval_list_constructor",
			"'al' is NULL"
		);
		return -1;
	}

	/* Find smallest power of 2 greater than or equal
	   to 'len' to use as allocation size */
	al_alloc = 1;

	while (al_alloc < len) {
		al_alloc *= 2;
	}

	/* Allocate space for values */
	al_vals = malloc(sizeof(struct simplescim_arbval *) * al_alloc);

	if (al_vals == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_arbval_list_constructor:"
			"malloc"
		);
		return -1;
	}

	/* Assign data to 'al' */
	al->al_len = 0;
	al->al_alloc = al_alloc;
	al->al_vals = al_vals;

	return 0;
}

/**
 * Copies the values of 'other' to 'al'. Both 'al' and
 * 'other' must be allocated.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_arbval_list_constructor_copy(
	struct simplescim_arbval_list *al,
	const struct simplescim_arbval_list *other
)
{
	struct simplescim_arbval *av;
	uint64_t i;
	int err;

	if (other == NULL) {
		simplescim_error_string_set(
			"simplescim_arbval_list_constructor_copy",
			"'other' is NULL"
		);
		return -1;
	}

	/* Allocate sufficient space in 'al' */
	err = simplescim_arbval_list_constructor(
		al,
		other->al_len
	);

	if (err == -1) {
		return -1;
	}

	/* Copy values */
	for (i = 0; i < other->al_len; ++i) {
		av = simplescim_arbval_copy(other->al_vals[i]);

		if (av == NULL) {
			simplescim_arbval_list_destructor(al);
			return -1;
		}

		err = simplescim_arbval_list_append(
			al,
			av
		);

		if (err == -1) {
			simplescim_arbval_delete(av);
			simplescim_arbval_list_destructor(al);
			return -1;
		}
	}

	return 0;
}

/**
 * Frees memory associated with 'al'.
 */
void simplescim_arbval_list_destructor(
	struct simplescim_arbval_list *al
)
{
	uint64_t i;

	if (al != NULL && al->al_vals != NULL) {
		for (i = 0; i < al->al_len; ++i) {
			simplescim_arbval_delete(al->al_vals[i]);
		}

		free(al->al_vals);
	}
}

/**
 * Allocates space for at least 'len' struct
 * simplescim_arbval pointers into a new struct
 * simplescim_arbval_list object.
 * On success, a pointer to the newly created object is
 * returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
struct simplescim_arbval_list *simplescim_arbval_list_new(
	uint64_t len
)
{
	struct simplescim_arbval_list *al;
	int err;

	al = malloc(sizeof(struct simplescim_arbval_list));

	if (al == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_arbval_list_new:"
			"malloc"
		);
		return NULL;
	}

	err = simplescim_arbval_list_constructor(
		al,
		len
	);

	if (err == -1) {
		free(al);
		return NULL;
	}

	return al;
}

/**
 * Copies the values of 'other' into a new struct
 * simplescim_arbval_list object.
 * On success, a pointer to the newly created object is
 * returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
struct simplescim_arbval_list *simplescim_arbval_list_copy(
	const struct simplescim_arbval_list *other
)
{
	struct simplescim_arbval_list *al;
	int err;

	al = malloc(sizeof(struct simplescim_arbval_list));

	if (al == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_arbval_list_copy:"
			"malloc"
		);
		return NULL;
	}

	err = simplescim_arbval_list_constructor_copy(
		al,
		other
	);

	if (err == -1) {
		free(al);
		return NULL;
	}

	return al;
}

/**
 * Frees 'al' and memory associated with 'al'.
 */
void simplescim_arbval_list_delete(
	struct simplescim_arbval_list *al
)
{
	if (al != NULL) {
		simplescim_arbval_list_destructor(al);
		free(al);
	}
}

/**
 * Appends 'av' to 'al'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_arbval_list_append(
	struct simplescim_arbval_list *al,
	struct simplescim_arbval *av
)
{
	uint64_t al_alloc;
	struct simplescim_arbval **al_vals;

	if (al == NULL) {
		simplescim_error_string_set(
			"simplescim_arbval_list_append",
			"'al' is NULL"
		);
		return -1;
	}

	/* Check that there is enough space for one more
	   element. If not, allocate more space. */
	if (al->al_len == al->al_alloc) {
		al_alloc = 2 * al->al_alloc;
		al_vals = realloc(
			al->al_vals,
			sizeof(struct simplescim_arbval *) * al_alloc
		);

		if (al_vals == NULL) {
			simplescim_error_string_set_errno(
				"simplescim_arbval_list_append:"
				"realloc"
			);
			return -1;
		}

		al->al_alloc = al_alloc;
		al->al_vals = al_vals;
	}

	/* Append 'av' */
	al->al_vals[al->al_len] = av;
	++al->al_len;

	return 0;
}

/**
 * Returns 1 if 'al1' ⊆ 'al2'. Returns 0 otherwise.
 */
int simplescim_arbval_list_subset_eq(
	const struct simplescim_arbval_list *al1,
	const struct simplescim_arbval_list *al2
)
{
	uint64_t i, j;

	/* If both are NULL, they are equal */
	if (al1 == NULL && al2 == NULL) {
		return 1;
	}

	/* If one is NULL, they are not equal */
	if (al1 == NULL || al2 == NULL) {
		return 0;
	}

	/* For every value in 'al1' */
	for (i = 0; i < al1->al_len; ++i) {
		/* Find equal value in 'al2' */
		for (j = 0; j < al2->al_len; ++j) {
			/* If values are equal, break out of loop */
			if (simplescim_arbval_eq(al1->al_vals[i],
			                         al2->al_vals[j])) {
				break;
			}
		}

		/* If the for-loop above was not terminated
		   early, no equal value was found in 'al2'. */
		if (j == al2->al_len) {
			return 0;
		}
	}

	return 1;
}

/**
 * Returns 1 if 'al1' = 'al2'. Returns 0 otherwise.
 */
int simplescim_arbval_list_eq(
	const struct simplescim_arbval_list *al1,
	const struct simplescim_arbval_list *al2
)
{
	return simplescim_arbval_list_subset_eq(al1, al2)
	       && simplescim_arbval_list_subset_eq(al2, al1);
}
