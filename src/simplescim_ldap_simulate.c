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

#include "simplescim_ldap.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "simplescim_error_string.h"
#include "simplescim_arbval.h"
#include "simplescim_arbval_list.h"
#include "simplescim_user.h"
#include "simplescim_user_list.h"

struct attribute {
	const char *name;
	size_t n_values;
	const char *values[10];
};

struct user {
	size_t n_attributes;
	struct attribute attributes[10];
};

static struct user g_users[] = {
	{4, {
		{"uid", 1, {"test1"}},
		{"fullName", 1, {"Test1 Testtest1"}},
		{"givenName", 1, {"Test1"}},
		{"email", 2, {
			"test1@example.com",
			"test1@email.com"
		}}
	}},
	{4, {
		{"uid", 1, {"test2"}},
		{"fullName", 1, {"Test2 Testtest2"}},
		{"givenName", 1, {"Test2"}},
		{"email", 3, {
			"test2@example.com",
			"test2@email.com",
			"test2@gmail.com"
		}}
	}}
};

static struct simplescim_arbval_list *get_values(struct attribute *ap)
{
	struct simplescim_arbval_list *values;
	struct simplescim_arbval *value;
	size_t i;
	int err;

	values = simplescim_arbval_list_new(ap->n_values);

	for (i = 0; i < ap->n_values; ++i) {
		value = simplescim_arbval_string(ap->values[i]);

		if (value == NULL) {
			simplescim_arbval_list_delete(values);
			return NULL;
		}

		err = simplescim_arbval_list_append(
			values,
			value
		);

		if (err == -1) {
			simplescim_arbval_delete(value);
			simplescim_arbval_list_delete(values);
			return NULL;
		}
	}

	return values;
}

static struct simplescim_user *get_user(struct user *up)
{
	struct simplescim_user *user;
	char *attribute;
	struct simplescim_arbval_list *values;
	size_t i;
	int err;

	user = simplescim_user_new();

	for (i = 0; i < up->n_attributes; ++i) {
		attribute = strdup(up->attributes[i].name);

		if (attribute == NULL) {
			simplescim_error_string_set_errno(
				"get_users:"
				"strdup"
			);
			simplescim_user_delete(user);
			return NULL;
		}

		values = get_values(&up->attributes[i]);

		if (values == NULL) {
			free(attribute);
			simplescim_user_delete(user);
			return NULL;
		}

		err = simplescim_user_set_attribute(
			user,
			attribute,
			values
		);

		if (err == -1) {
			simplescim_arbval_list_delete(values);
			free(attribute);
			simplescim_user_delete(user);
			return NULL;
		}
	}

	return user;
}

struct simplescim_user_list *simplescim_ldap_get_users()
{
	struct simplescim_user_list *users;
	struct simplescim_user *user;
	struct simplescim_arbval *uid;
	size_t i;
	int err;

	users = simplescim_user_list_new();

	if (users == NULL) {
		return NULL;
	}

	for (i = 0; i < sizeof(g_users) / sizeof(struct user); ++i) {
		user = get_user(&g_users[i]);

		if (user == NULL) {
			simplescim_user_list_delete(users);
			return NULL;
		}

		uid = simplescim_user_get_uid(user);

		if (uid == NULL) {
			simplescim_user_delete(user);
			simplescim_user_list_delete(users);
			return NULL;
		}

		err = simplescim_user_list_insert_user(
			users,
			uid,
			user
		);

		if (err == -1) {
			simplescim_arbval_delete(uid);
			simplescim_user_delete(user);
			simplescim_user_list_delete(users);
			return NULL;
		}
	}

	return users;
}
