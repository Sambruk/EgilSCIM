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

#include "simplescim_arbval.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "simplescim_error_string.h"

/**
 * Copies 'len' bytes from 'val' to 'av'. 'av' must be
 * allocated.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_arbval_constructor(
	struct simplescim_arbval *av,
	uint64_t len,
	const uint8_t *val
)
{
	uint8_t *av_val;

	if (av == NULL) {
		simplescim_error_string_set(
			"simplescim_arbval_constructor",
			"'av' is NULL"
		);
		return -1;
	}

	if (val == NULL) {
		av_val = NULL;
	} else {
		av_val = malloc(len + 1);

		if (av_val == NULL) {
			simplescim_error_string_set_errno(
				"simplescim_arbval_constructor:"
				"malloc"
			);
			return -1;
		}

		memcpy(av_val, val, len);
		av_val[len] = '\0';
	}

	av->av_len = len;
	av->av_val = av_val;

	return 0;
}

/**
 * Copies the value of 'other' to 'av'. Both 'av' and
 * 'other' must be allocated.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_arbval_constructor_copy(
	struct simplescim_arbval *av,
	const struct simplescim_arbval *other
)
{
	int err;

	if (other == NULL) {
		simplescim_error_string_set(
			"simplescim_arbval_constructor_copy",
			"'other' is NULL"
		);
		return -1;
	}

	err = simplescim_arbval_constructor(
		av,
		other->av_len,
		other->av_val
	);

	if (err == -1) {
		return -1;
	}

	return 0;
}

/**
 * Copies the string 'str' to 'av'. 'av' must be allocated.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_arbval_constructor_string(
	struct simplescim_arbval *av,
	const char *str
)
{
	int err;

	if (str == NULL) {
		err = simplescim_arbval_constructor(
			av,
			0,
			NULL
		);

		if (err == -1) {
			return -1;
		}
	} else {
		err = simplescim_arbval_constructor(
			av,
			strlen(str),
			(const uint8_t *)str
		);

		if (err == -1) {
			return -1;
		}
	}

	return 0;
}

/**
 * Frees memory associated with 'av'.
 */
void simplescim_arbval_destructor(
	struct simplescim_arbval *av
)
{
	if (av != NULL && av->av_val != NULL) {
		free(av->av_val);
	}
}

/**
 * Copies 'len' bytes from 'val' into a new struct
 * simplescim_arbval object.
 * On success, a pointer to the newly created object is
 * returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
struct simplescim_arbval *simplescim_arbval_new(
	uint64_t len,
	const uint8_t *val
)
{
	struct simplescim_arbval *av;
	int err;

	av = malloc(sizeof(struct simplescim_arbval));

	if (av == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_arbval_new:"
			"malloc"
		);
		return NULL;
	}

	err = simplescim_arbval_constructor(
		av,
		len,
		val
	);

	if (err == -1) {
		free(av);
		return NULL;
	}

	return av;
}

/**
 * Copies the value of 'other' into a new struct
 * simplescim_arbval object. 'other' must be allocated.
 * On success, a pointer to the newly created object is
 * returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
struct simplescim_arbval *simplescim_arbval_copy(
	const struct simplescim_arbval *other
)
{
	struct simplescim_arbval *av;
	int err;

	av = malloc(sizeof(struct simplescim_arbval));

	if (av == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_arbval_copy:"
			"malloc"
		);
		return NULL;
	}

	err = simplescim_arbval_constructor_copy(
		av,
		other
	);

	if (err == -1) {
		free(av);
		return NULL;
	}

	return av;
}

/**
 * Copies the string 'str' into a new struct
 * simplescim_arbval object.
 * On success, a pointer to the newly created object is
 * returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
struct simplescim_arbval *simplescim_arbval_string(
	const char *str
)
{
	struct simplescim_arbval *av;
	int err;

	av = malloc(sizeof(struct simplescim_arbval));

	if (av == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_arbval_string:"
			"malloc"
		);
		return NULL;
	}

	err = simplescim_arbval_constructor_string(
		av,
		str
	);

	if (err == -1) {
		free(av);
		return NULL;
	}

	return av;
}

/**
 * Frees 'av' and memory associated with 'av'.
 */
void simplescim_arbval_delete(
	struct simplescim_arbval *av
)
{
	if (av != NULL) {
		simplescim_arbval_destructor(av);
		free(av);
	}
}

/**
 * Returns 1 if 'av1' = 'av2'. Returns 0 otherwise.
 */
int simplescim_arbval_eq(
	const struct simplescim_arbval *av1,
	const struct simplescim_arbval *av2
)
{
	uint64_t i;

	/* If both are NULL, they're equal */
	if (av1 == NULL && av2 == NULL) {
		return 1;
	}

	/* If one is NULL, they're not equal */
	if (av1 == NULL || av2 == NULL) {
		return 0;
	}

	/* If they have different lengths, they're not equal */
	if (av1->av_len != av2->av_len) {
		return 0;
	}

	/* If every byte is equal, they're equal */
	for (i = 0; i < av1->av_len; ++i) {
		/* If one byte is different, they're not equal */
		if (av1->av_val[i] != av2->av_val[i]) {
			return 0;
		}
	}

	return 1;
}
