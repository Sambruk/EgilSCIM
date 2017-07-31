#include "simplescim_cache_file.h"

#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <lber.h>

#include "simplescim_error_string.h"
#include "simplescim_user.h"
#include "simplescim_user_list.h"
#include "simplescim_config_file.h"

/*************
 * Read part *
 *************/

/**
 * Frees all memory associated with a struct berval
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
 * of pointers to struct berval objects.
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
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int read_uint64(int fd,
                       const char *cache_file_name,
                       uint64_t *np)
{
	uint64_t n;
	ssize_t nread;

	nread = read(fd, &n, sizeof(uint64_t));

	if (nread == -1) {
		simplescim_error_string_set_errno(
			"%s",
			cache_file_name
		);
		return -1;
	}

	if ((size_t)nread < sizeof(uint64_t)) {
		simplescim_error_string_set_prefix(
			"%s",
			cache_file_name
		);
		simplescim_error_string_set_message(
			"unexpected end-of-file"
		);
		return -1;
	}

	*np = n;

	return 0;
}

/**
 * Reads 'n' bytes into 'buf' from 'fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
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
		simplescim_error_string_set_errno(
			"%s",
			cache_file_name
		);
		return -1;
	}

	if ((size_t)nread < n) {
		simplescim_error_string_set_prefix(
			"%s",
			cache_file_name
		);
		simplescim_error_string_set_message(
			"unexpected end-of-file"
		);
		return -1;
	}

	return 0;
}

/**
 * Reads a struct berval in the cache file from 'fd' and
 * stores it in 'berp'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
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
		simplescim_error_string_set_errno(
			"read_berval:malloc"
		);
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
		simplescim_error_string_set_errno(
			"read_berval:malloc"
		);
		free(bv_val);
		return -1;
	}

	/* Assign member variables to 'ber' */
	ber->bv_len = bv_len;
	ber->bv_val = (char *)bv_val;

	/* Store 'ber' in 'berp' */
	*berp = ber;

	return 0;
}

/**
 * Reads a NULL-terminated list of values in the cache file
 * from 'fd' and stores them in 'valuesp'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
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
		simplescim_error_string_set_errno(
			"read_values:malloc"
		);
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
 * and simplescim_error_string is set to an appropriate
 * error message.
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

	/* Allocate user object */
	user = simplescim_user_new();

	if (user == NULL) {
		delete_berval(unique_identifier);
		return -1;
	}

	/* Read attributes */
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
 * On success, a pointer to a newly created user list
 * object is returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate
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
 * Reads cache file specified in configuration file and
 * constructs a user list according to its contents.
 * On success, a pointer to the constructed user list is
 * returned. If the cache file doesn't exist, an empty user
 * list is returned. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
struct simplescim_user_list *simplescim_cache_file_get_users()
{
	struct simplescim_user_list *users;
	const char *cache_file_name;
	int fd;

	/* Get cache file name from configuration file */
	simplescim_config_file_get("cache-file", &cache_file_name);

	/* Check if cache file exists.
	   If not, return an empty user list. */
	if (access(cache_file_name, F_OK) != 0) {
		return simplescim_user_list_new();
	}

	/* Open cache file */
	fd = open(cache_file_name, O_RDONLY);

	if (fd == -1) {
		simplescim_error_string_set_errno(
			"%s",
			cache_file_name
		);
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

/**************
 * Write part *
 **************/

static int cache_fd;

/**
 * Writes one user attribute to 'cache_fd'.
 */
static void write_attr(const char *attribute,
                       const struct berval **values)
{
	uint64_t attr_len;
	uint64_t n_vals;
	uint64_t val_len;
	uint8_t *val;
	size_t i;

	/* Calculate attribute_name_length */
	attr_len = strlen(attribute);

	/* Write attribute_name_length and attribute_name */
	write(cache_fd, &attr_len, sizeof(uint64_t));
	write(cache_fd, attribute, attr_len);

	/* Calculate n_values */
	n_vals = 0;

	if (values != NULL) {
		while (values[n_vals] != NULL) {
			++n_vals;
		}
	}

	/* Write n_values */
	write(cache_fd, &n_vals, sizeof(uint64_t));

	/* Write values */
	for (i = 0; i < n_vals; ++i) {
		/* Get value_length and value */
		val_len = values[i]->bv_len;
		val = (uint8_t *)values[i]->bv_val;

		/* Write value_length and value */
		write(cache_fd, &val_len, sizeof(uint64_t));
		write(cache_fd, val, val_len);
	}
}

/**
 * Writes one user to 'cache_fd'.
 */
static void write_user(const struct berval *unique_identifier,
                       const struct simplescim_user *user)
{
	uint64_t uid_len;
	uint8_t *uid;
	uint64_t n_attrs;

	/* Get unique_identifier_length and unique_identifier */
	uid_len = unique_identifier->bv_len;
	uid = (uint8_t *)unique_identifier->bv_val;

	/* Write unique_identifier_length and unique_identifier */
	write(cache_fd, &uid_len, sizeof(uint64_t));
	write(cache_fd, uid, uid_len);

	/* Get n_attributes */
	n_attrs = simplescim_user_get_n_attributes(user);

	/* Write n_attributes and attributes */
	write(cache_fd, &n_attrs, sizeof(uint64_t));
	simplescim_user_foreach(user, write_attr);
}

/**
 * Writes 'users' to cache file specified in configuration
 * file.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_cache_file_save(const struct simplescim_user_list *users)
{
	int fd;
	const char *cache_file_name;
	uint64_t n_users;

	/* Get cache file's name */
	simplescim_config_file_get("cache-file", &cache_file_name);

	/* Open cache file for writing (mode 0664) */
	fd = open(cache_file_name,
	          O_WRONLY | O_CREAT | O_TRUNC,
	          S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

	if (fd == -1) {
		simplescim_error_string_set_errno(
			"%s",
			cache_file_name
		);
		return -1;
	}

	cache_fd = fd;

	/* Get n_users */
	n_users = simplescim_user_list_get_n_users(users);

	/* Write n_users and users */
	write(fd, &n_users, sizeof(uint64_t));
	simplescim_user_list_foreach(users, write_user);

	close(fd);

	return 0;
}
