#include "simplescim_cache_file.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <lber.h>

#include "simplescim_error_string.h"
#include "simplescim_config_file.h"
#include "simplescim_open_file.h"
#include "simplescim_user.h"
#include "simplescim_user_list.h"

/**
 * Frees all memory associated with a 'struct berval'
 * object.
 */
static void delete_berval(struct berval *ber)
{
	if (ber != NULL) {
		if (ber->bv_val != NULL) {
			free(ber->bv_val);
		}

		free(ber);
	}
}

/**
 * Frees all memory associated with a NULL-terminated list
 * of pointers to 'struct berval' objects.
 */
static void delete_values(struct berval **values)
{
	size_t i;

	if (values != NULL) {
		for (i = 0; values[i] != NULL; ++i) {
			delete_berval(values[i]);
		}

		free(values);
	}
}

/**
 * Reads a 64-bit value in the cache file from 'fd' and
 * stores it in 'np'.
 * On success, zero is returned. On error, -1 is returned
 * and 'simplescim_error_string' is set to an appropriate
 * error message. 'n' will in that case remain untouched.
 */
static int read_uint64(int fd,
                       const char *cache_file_name,
                       uint64_t *np)
{
	uint64_t n;
	ssize_t nread;

	nread = read(fd, &n, sizeof(uint64_t));

	if (nread == -1) {
		sprintf(simplescim_error_string,
		        "%s:%s: %s",
		        simplescim_config_file_name,
		        cache_file_name,
		        strerror(errno));
		return -1;
	}

	if ((size_t)nread < sizeof(uint64_t)) {
		sprintf(simplescim_error_string,
		        "%s:%s: unexpected end-of-file",
		        simplescim_config_file_name,
		        cache_file_name);
		return -1;
	}

	*np = n;

	return 0;
}

/**
 * Reads 'n' bytes into 'buf' from 'fd'.
 * On success, zero is returned. On error, -1 is returned
 * and 'simplescim_error_string' is set to an appropriate
 * error message.
 */
static int read_n(int fd,
                  const char *cache_file_name,
                  uint8_t *buf,
                  size_t n)
{
	ssize_t nread;

	nread = read(fd, buf, n);

	if (nread == -1) {
		sprintf(simplescim_error_string,
		        "%s:%s: %s",
		        simplescim_config_file_name,
		        cache_file_name,
		        strerror(errno));
		return -1;
	}

	if ((size_t)nread < n) {
		sprintf(simplescim_error_string,
		        "%s:%s: unexpected end-of-file",
		        simplescim_config_file_name,
		        cache_file_name);
		return -1;
	}

	return 0;
}

/**
 * Reads a struct berval in the cache file from 'fd' and
 * stores it in 'berp'.
 * On success, zero is returned. On error, -1 is returned
 * and 'simplescim_error_string' is set to an appropriate
 * error message. 'unique_identifierp' and 'userp' will in
 * that case remain untouched.
 */
static int read_berval(int fd,
                       const char *cache_file_name,
                       struct berval **berp)
{
	struct berval *ber;
	uint64_t bv_len;
	uint8_t *bv_val;
	int err;

	/* Read value length */

	err = read_uint64(fd, cache_file_name, &bv_len);

	if (err == -1) {
		return -1;
	}

	/* Allocate value */

	bv_val = malloc(bv_len + 1);

	if (bv_val == NULL) {
		simplescim_error_string_malloc();
		return -1;
	}

	/* Read value */

	err = read_n(fd, cache_file_name, bv_val, bv_len);

	if (err == -1) {
		free(bv_val);
		return -1;
	}

	/* Null-terminate value */

	bv_val[bv_len] = '\0';

	/* Allocate 'struct berval *' */

	ber = malloc(sizeof(struct berval));

	if (ber == NULL) {
		simplescim_error_string_malloc();
		free(bv_val);
		return -1;
	}

	/* Assign member variables to 'struct berval *' */

	ber->bv_len = bv_len;
	ber->bv_val = (char *)bv_val;

	/* Store 'struct berval *' in 'berp' */

	*berp = ber;

	return 0;
}

/**
 * Reads a NULL-terminated list of values in the cache file
 * from 'fd' and stores them in 'valuesp'.
 * On success, zero is returned. On error, -1 is returned
 * and 'simplescim_error_string' is set to an appropriate
 * error message. 'valuesp' will in that case remain
 * untouched.
 */
static int read_values(int fd,
                       const char *cache_file_name,
                       struct berval ***valuesp)
{
	struct berval **values;
	struct berval *value;
	uint64_t n_values;
	size_t i;
	int err;

	/* Read number of values */

	err = read_uint64(fd, cache_file_name, &n_values);

	if (err == -1) {
		return -1;
	}

	/* Allocate list of values */

	values = malloc(sizeof(struct berval *) * (n_values + 1));

	if (values == NULL) {
		simplescim_error_string_malloc();
		return -1;
	}

	/* Read the values */

	for (i = 0; i < n_values; ++i) {
		/* Read value */

		err = read_berval(fd, cache_file_name, &value);

		if (err == -1) {
			/* Delete all values so far */

			while (i > 0) {
				delete_berval(values[i - 1]);
				--i;
			}

			free(values);

			return -1;
		}

		/* Insert read value into values list */

		values[i] = value;
	}

	values[n_values] = NULL;
	*valuesp = values;

	return 0;
}

/**
 * Reads one user in the cache file from 'fd' and stores it
 * in 'userp', and stores the user's unique identifier in
 * 'unique_identifierp'.
 * On success, zero is returned. On error, -1 is returned
 * and 'simplescim_error_string' is set to an appropriate
 * error message. 'unique_identifierp' and 'userp' will in
 * that case remain untouched.
 */
static int read_user(int fd,
                     const char *cache_file_name,
                     struct berval **unique_identifierp,
                     struct simplescim_user **userp)
{
	struct berval *unique_identifier;
	struct simplescim_user *user;
	uint64_t n_attributes;
	struct berval *attribute;
	struct berval **values;
	size_t i;
	int err;

	/* Read user's unique identifier */

	err = read_berval(fd, cache_file_name, &unique_identifier);

	if (err == -1) {
		return -1;
	}

	/* Read n_attributes */

	err = read_uint64(fd, cache_file_name, &n_attributes);

	if (err == -1) {
		delete_berval(unique_identifier);
		return -1;
	}

	/* Read attributes */

	user = simplescim_user_new();

	if (user == NULL) {
		delete_berval(unique_identifier);
		return -1;
	}

	for (i = 0; i < n_attributes; ++i) {
		/* Read attribute name */

		err = read_berval(fd, cache_file_name, &attribute);

		if (err == -1) {
			simplescim_user_delete(user);
			delete_berval(unique_identifier);
			return -1;
		}

		/* Read values */

		err = read_values(fd, cache_file_name, &values);

		if (err == -1) {
			delete_berval(attribute);
			simplescim_user_delete(user);
			delete_berval(unique_identifier);
			return -1;
		}

		/* Insert into user object */

		err = simplescim_user_set_attribute(
			user,
			attribute->bv_val,
			values
		);

		if (err == -1) {
			delete_values(values);
			delete_berval(attribute);
			simplescim_user_delete(user);
			delete_berval(unique_identifier);
			return -1;
		}

		free(attribute);
	}

	*unique_identifierp = unique_identifier;
	*userp = user;

	return 0;
}

/**
 * Read the cache file's contents from 'fd'.
 * On success, a pointer to a newly creates user list
 * object is returned. On error, NULL is returned and
 * 'simplescim_error_string' is set to an appropriate
 * error message.
 */
static struct simplescim_user_list *
read_cache_file(int fd, const char *cache_file_name)
{
	struct simplescim_user_list *users;
	struct berval *unique_identifier;
	struct simplescim_user *user;
	uint64_t n_users;
	size_t i;
	int err;

	/* Read n_users */

	err = read_uint64(fd, cache_file_name, &n_users);

	if (err == -1) {
		return NULL;
	}

	/* Allocate user list */

	users = simplescim_user_list_new();

	if (users == NULL) {
		return NULL;
	}

	/* Read all users */

	for (i = 0; i < n_users; ++i) {
		/* Read user */

		err = read_user(fd,
		                cache_file_name,
		                &unique_identifier,
		                &user);

		if (err == -1) {
			simplescim_user_list_delete(users);
			return NULL;
		}

		/* Insert user into user list */

		err = simplescim_user_list_insert_user(
			users,
			unique_identifier,
			user
		);

		if (err == -1) {
			delete_berval(unique_identifier);
			simplescim_user_delete(user);
			simplescim_user_list_delete(users);
			return NULL;
		}
	}

	return users;
}

/**
 * Reads cache file specified in global configuration file
 * structure and constructs a user list according to its
 * contents.
 * On success, a pointer to the constructed user list is
 * returned. If the cache file doesn't exist, an empty user
 * list is returned. On error, NULL is returned and
 * 'simplescim_error_string' is set to an appropriate error
 * message.
 */
struct simplescim_user_list *simplescim_cache_file_load()
{
	struct simplescim_user_list *users;
	const char *cache_file_name;
	int fd;

	/* Get cache file name from configuration file */

	simplescim_config_file_get("cache-file", &cache_file_name);

	/* TODO: Check if cache file exists. If not, return
	         an empty user list. */

	/* Open cache file */

	fd = simplescim_open_file(cache_file_name, NULL);

	if (fd == -1) {
		return NULL;
	}

	/* Read user list from cache file */

	users = read_cache_file(fd, cache_file_name);

	if (users == NULL) {
		close(fd);
		return NULL;
	}

	close(fd);

	return users;
}
