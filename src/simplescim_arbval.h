#ifndef SIMPLESCIM_ARBVAL_H
#define SIMPLESCIM_ARBVAL_H

#include <stdint.h>

struct simplescim_arbval {
	uint64_t av_len;
	uint8_t *av_val;
};

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
);

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
);

/**
 * Copies the string 'str' to 'av'. 'av' must be allocated.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_arbval_constructor_string(
	struct simplescim_arbval *av,
	const char *str
);

/**
 * Frees memory associated with 'av'.
 */
void simplescim_arbval_destructor(
	struct simplescim_arbval *av
);

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
);

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
);

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
);

/**
 * Frees 'av' and memory associated with 'av'.
 */
void simplescim_arbval_delete(
	struct simplescim_arbval *av
);

/**
 * Returns 1 if 'av1' = 'av2'. Returns 0 otherwise.
 */
int simplescim_arbval_eq(
	const struct simplescim_arbval *av1,
	const struct simplescim_arbval *av2
);

#endif
