#ifndef SIMPLESCIM_ARBVAL_LIST_H
#define SIMPLESCIM_ARBVAL_LIST_H

#include <stdint.h>

#include "simplescim_arbval.h"

struct simplescim_arbval_list {
	uint64_t al_len;
	uint64_t al_alloc;
	struct simplescim_arbval **al_vals;
};

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
);

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
);

/**
 * Frees memory associated with 'al'.
 */
void simplescim_arbval_list_destructor(
	struct simplescim_arbval_list *al
);

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
);

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
);

/**
 * Frees 'al' and memory associated with 'al'.
 */
void simplescim_arbval_list_delete(
	struct simplescim_arbval_list *al
);

/**
 * Appends 'av' to 'al'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_arbval_list_append(
	struct simplescim_arbval_list *al,
	struct simplescim_arbval *av
);

/**
 * Returns 1 if 'al1' ⊆ 'al2'. Returns 0 otherwise.
 */
int simplescim_arbval_list_subset_eq(
	const struct simplescim_arbval_list *al1,
	const struct simplescim_arbval_list *al2
);

/**
 * Returns 1 if 'al1' = 'al2'. Returns 0 otherwise.
 */
int simplescim_arbval_list_eq(
	const struct simplescim_arbval_list *al1,
	const struct simplescim_arbval_list *al2
);

#endif
