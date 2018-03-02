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

#include "simplescim_cache_file.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "simplescim_error_string.h"
#include "simplescim_arbval.h"
#include "simplescim_arbval_list.h"
#include "simplescim_user.h"
#include "simplescim_user_list.h"
#include "simplescim_config_file.h"

/**
 * Reads 'n' bytes into 'buf' from 'fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_cache_file_read_n(
	int fd,
	const char *filename,
	uint8_t *buf,
	size_t n
)
{
	ssize_t nread;

	nread = read(fd, buf, n);

	if (nread == -1) {
		simplescim_error_string_set_errno(
			"simplescim_cache_file_read_n:read:%s",
			filename
		);
		return -1;
	}

	if ((size_t)nread < n) {
		simplescim_error_string_set_prefix(
			"simplescim_cache_file_read_n:read:%s",
			filename
		);
		simplescim_error_string_set_message(
			"unexpected end-of-file"
		);
		return -1;
	}

	return 0;
}

/**
 * Reads a 64-bit value from 'fd' and stores it in 'buf'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_cache_file_read_uint64(
	int fd,
	const char *filename,
	uint64_t *buf
)
{
	int err;

	err = simplescim_cache_file_read_n(
		fd,
		filename,
		(uint8_t *)buf,
		sizeof(uint64_t)
	);

	if (err == -1) {
		return -1;
	}

	return 0;
}

/**
 * Reads a struct simplescim_arbval from 'fd' and stores it
 * in 'avp'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_cache_file_read_arbval(
	int fd,
	const char *filename,
	struct simplescim_arbval **avp
)
{
	struct simplescim_arbval *av;
	uint64_t av_len;
	uint8_t *av_val;
	int err;

	/* Read value length */
	err = simplescim_cache_file_read_uint64(
		fd,
		filename,
		&av_len
	);

	if (err == -1) {
		return -1;
	}

	/* Allocate value */
	av_val = malloc(av_len + 1);

	if (av_val == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_cache_file_read_arbval:"
			"malloc"
		);
		return -1;
	}

	/* Read value */
	err = simplescim_cache_file_read_n(
		fd,
		filename,
		av_val,
		av_len
	);

	if (err == -1) {
		free(av_val);
		return -1;
	}

	/* Null-terminate value */
	av_val[av_len] = '\0';

	/* Allocate 'av' */
	av = simplescim_arbval_new(
		0,
		NULL
	);

	if (av == NULL) {
		free(av_val);
		return -1;
	}

	av->av_len = av_len;
	av->av_val = av_val;
	*avp = av;

	return 0;
}

/**
 * Reads a struct simplescim_arbval_list from 'fd' and
 * stores it in 'alp'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_cache_file_read_arbval_list(
	int fd,
	const char *filename,
	struct simplescim_arbval_list **alp
)
{
	struct simplescim_arbval_list *al;
	struct simplescim_arbval *av;
	uint64_t al_len;
	uint64_t i;
	int err;

	/* Read number of values */
	err = simplescim_cache_file_read_uint64(
		fd,
		filename,
		&al_len
	);

	if (err == -1) {
		return -1;
	}

	/* Allocate 'al' */
	al = simplescim_arbval_list_new(al_len);

	if (al == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_cache_file_read_arbval_list:"
			"malloc"
		);
		return -1;
	}

	/* Read all values */
	for (i = 0; i < al_len; ++i) {
		/* Read 'av' */
		err = simplescim_cache_file_read_arbval(
			fd,
			filename,
			&av
		);

		if (err == -1) {
			simplescim_arbval_list_delete(al);
			return -1;
		}

		/* Insert 'av' into 'al' */
		err = simplescim_arbval_list_append(
			al,
			av
		);

		if (err == -1) {
			simplescim_arbval_delete(av);
			simplescim_arbval_list_delete(al);
			return -1;
		}
	}

	*alp = al;

	return 0;
}

/**
 * Reads one user from 'fd' and stores it in 'userp', and
 * stores the user's unique identifier in 'uidp'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_cache_file_read_user(
	int fd,
	const char *filename,
	struct simplescim_arbval **uidp,
	struct simplescim_user **userp
)
{
	struct simplescim_arbval *uid;
	struct simplescim_user *user;
	uint64_t n_attributes;
	uint64_t i;
	struct simplescim_arbval *attribute;
	struct simplescim_arbval_list *values;
	int err;

	/* Read user's unique identifier */
	err = simplescim_cache_file_read_arbval(
		fd,
		filename,
		&uid
	);

	if (err == -1) {
		return -1;
	}

	/* Read n_attributes */
	err = simplescim_cache_file_read_uint64(
		fd,
		filename,
		&n_attributes
	);

	if (err == -1) {
		simplescim_arbval_delete(uid);
		return -1;
	}

	/* Allocate user object */
	user = simplescim_user_new();

	if (user == NULL) {
		simplescim_arbval_delete(uid);
		return -1;
	}

	/* Read attributes */
	for (i = 0; i < n_attributes; ++i) {
		/* Read attribute name */
		err = simplescim_cache_file_read_arbval(
			fd,
			filename,
			&attribute
		);

		if (err == -1) {
			simplescim_user_delete(user);
			simplescim_arbval_delete(uid);
			return -1;
		}

		/* Read values */
		err = simplescim_cache_file_read_arbval_list(
			fd,
			filename,
			&values
		);

		if (err == -1) {
			simplescim_arbval_delete(attribute);
			simplescim_user_delete(user);
			simplescim_arbval_delete(uid);
			return -1;
		}

		/* Insert into user object */
		err = simplescim_user_set_attribute(
			user,
			(char *)attribute->av_val,
			values
		);

		if (err == -1) {
			simplescim_arbval_list_delete(values);
			simplescim_arbval_delete(attribute);
			simplescim_user_delete(user);
			simplescim_arbval_delete(uid);
			return -1;
		}

		attribute->av_val = NULL;
		simplescim_arbval_delete(attribute);
	}

	*uidp = uid;
	*userp = user;

	return 0;
}

/**
 * Reads users from 'fd' and stores them in 'usersp'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_cache_file_read_users(
	int fd,
	const char *filename,
	struct simplescim_user_list **usersp
)
{
	struct simplescim_user_list *users;
	struct simplescim_arbval *uid;
	struct simplescim_user *user;
	uint64_t n_users;
	uint64_t i;
	int err;

	/* Read n_users */
	err = simplescim_cache_file_read_uint64(
		fd,
		filename,
		&n_users
	);

	if (err == -1) {
		return -1;
	}

	/* Allocate user list */
	users = simplescim_user_list_new();

	if (users == NULL) {
		return -1;
	}

	/* Read all users */
	for (i = 0; i < n_users; ++i) {
		/* Read user */
		err = simplescim_cache_file_read_user(
			fd,
			filename,
			&uid,
			&user
		);

		if (err == -1) {
			simplescim_user_list_delete(users);
			return -1;
		}

		/* Insert user into user list */
		err = simplescim_user_list_insert_user(
			users,
			uid,
			user
		);

		if (err == -1) {
			simplescim_user_delete(user);
			simplescim_arbval_delete(uid);
			simplescim_user_list_delete(users);
			return -1;
		}
	}

	*usersp = users;

	return 0;
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
	const char *filename;
	int err;

	/* Get the cache file's name from configuration file */
	err = simplescim_config_file_get(
		"cache-file",
		&filename
	);

	if (err == -1) {
		simplescim_error_string_set(
			"simplescim_cache_file_get_users",
			"required variable \"cache-file\" is missing"
		);
		return NULL;
	}

	users = simplescim_cache_file_get_users_from_file(
		filename
	);

	if (users == NULL) {
		return NULL;
	}

	return users;
}

struct simplescim_user_list *simplescim_cache_file_get_users_from_file(
	const char *filename
)
{
	struct simplescim_user_list *users;
	int fd;
	int err;

	/* Check if cache file exists.
	   If not, return an empty user list. */
	if (access(filename, F_OK) == -1) {
		return simplescim_user_list_new();
	}

	/* Open cache file */
	fd = open(filename, O_RDONLY);

	if (fd == -1) {
		simplescim_error_string_set_errno(
			"simplescim_cache_file_get_users:open:%s",
			filename
		);
		return NULL;
	}

	/* Read user list from cache file */
	err = simplescim_cache_file_read_users(
		fd,
		filename,
		&users
	);

	if (err == -1) {
		close(fd);
		return NULL;
	}

	close(fd);

	return users;
}

static int simplescim_cache_file_fd;
static const char *simplescim_cache_file_filename;

/**
 * Writes 'n' bytes from 'buf' to
 * 'simplescim_cache_file_fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_cache_file_write_n(
	const uint8_t *buf,
	size_t n
)
{
	ssize_t nwritten;

	nwritten = write(
		simplescim_cache_file_fd,
		buf,
		n
	);

	if (nwritten == -1) {
		simplescim_error_string_set_errno(
			"simplescim_cache_file_write_n:write:%s",
			simplescim_cache_file_filename
		);
		return -1;
	}

	if ((size_t)nwritten < n) {
		simplescim_error_string_set_prefix(
			"simplescim_cache_file_write_n:write:%s",
			simplescim_cache_file_filename
		);
		simplescim_error_string_set_message(
			"could only write %ld B out of %lu B",
			nwritten,
			n
		);
		return -1;
	}

	return 0;
}

/**
 * Writes a 64-bit value from 'buf' to
 * 'simplescim_cache_file_fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_cache_file_write_uint64(
	uint64_t n
)
{
	int err;

	err = simplescim_cache_file_write_n(
		(const uint8_t *)&n,
		sizeof(uint64_t)
	);

	if (err == -1) {
		return -1;
	}

	return 0;
}

/**
 * Writes a struct simplescim_arbval from 'av' to
 * 'simplescim_cache_file_fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_cache_file_write_arbval(
	const struct simplescim_arbval *av
)
{
	int err;

	/* Write value_length */
	err = simplescim_cache_file_write_uint64(
		av->av_len
	);

	if (err == -1) {
		return -1;
	}

	/* Write value */
	err = simplescim_cache_file_write_n(
		av->av_val,
		av->av_len
	);

	if (err == -1) {
		return -1;
	}

	return 0;
}

/**
 * Writes a struct simplescim_arbval_list from 'al' to
 * 'simplescim_cache_file_fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_cache_file_write_arbval_list(
	const struct simplescim_arbval_list *al
)
{
	uint64_t i;
	int err;

	/* Write n_values */
	err = simplescim_cache_file_write_uint64(
		al->al_len
	);

	if (err == -1) {
		return -1;
	}

	/* Write values */
	for (i = 0; i < al->al_len; ++i) {
		err = simplescim_cache_file_write_arbval(
			al->al_vals[i]
		);

		if (err == -1) {
			return -1;
		}
	}

	return 0;
}

/**
 * Writes one user attribute and its values to
 * 'simplescim_cache_file_fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_cache_file_write_attribute(
	const char *attribute,
	const struct simplescim_arbval_list *values
)
{
	uint64_t attribute_len;
	int err;

	/* Calculate attribute_name_length */
	attribute_len = strlen(attribute);

	/* Write attribute_name_length */
	err = simplescim_cache_file_write_uint64(
		attribute_len
	);

	if (err == -1) {
		return -1;
	}

	/* Write attribute_name */
	err = simplescim_cache_file_write_n(
		(const uint8_t *)attribute,
		attribute_len
	);

	if (err == -1) {
		return -1;
	}

	/* Write n_values and values */
	err = simplescim_cache_file_write_arbval_list(
		values
	);

	if (err == -1) {
		return -1;
	}

	return 0;
}

/**
 * Writes one user to 'simplescim_cache_file_fd'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_cache_file_write_user(
	const struct simplescim_arbval *uid,
	const struct simplescim_user *user
)
{
	uint64_t n_attrs;
	int err;

	/* Write unique_identifier_length and unique_identifier */
	err = simplescim_cache_file_write_arbval(
		uid
	);

	if (err == -1) {
		return -1;
	}

	/* Get n_attributes */
	n_attrs = simplescim_user_get_n_attributes(user);

	/* Write n_attributes */
	err = simplescim_cache_file_write_uint64(
		n_attrs
	);

	if (err == -1) {
		return -1;
	}

	/* Write attributes */
	err = simplescim_user_foreach(
		user,
		simplescim_cache_file_write_attribute
	);

	if (err == -1) {
		return -1;
	}

	return 0;
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
	const char *filename;
	uint64_t n_users;
	int err;

	/* Get cache file's name */
	err = simplescim_config_file_get(
		"cache-file",
		&filename
	);

	if (err == -1) {
		simplescim_error_string_set(
			"simplescim_cache_file_save",
			"required variable \"cache-file\" is missing"
		);
		return -1;
	}

	/* Open cache file for writing (mode 0664) */
	fd = open(filename,
	          O_WRONLY | O_CREAT | O_TRUNC,
	          S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH);

	if (fd == -1) {
		simplescim_error_string_set_errno(
			"simplescim_cache_file_save:open:%s",
			filename
		);
		return -1;
	}

	simplescim_cache_file_fd = fd;
	simplescim_cache_file_filename = filename;

	/* Get n_users */
	n_users = simplescim_user_list_get_n_users(users);

	/* Write n_users */
	err = simplescim_cache_file_write_uint64(
		n_users
	);

	if (err == -1) {
		close(fd);
		return -1;
	}

	err = simplescim_user_list_foreach(
		users,
		simplescim_cache_file_write_user
	);

	if (err == -1) {
		close(fd);
		return -1;
	}

	close(fd);

	return 0;
}
