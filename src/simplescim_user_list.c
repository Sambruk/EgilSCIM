#include "simplescim_user_list.h"

#include <stdlib.h>
#include "uthash.h"

#include "simplescim_error_string.h"
#include "simplescim_arbval.h"
#include "simplescim_user.h"

struct user_record {
	struct simplescim_arbval *uid;
	struct simplescim_user *user;
	UT_hash_handle hh;
};

struct simplescim_user_list {
	size_t n_users;
	struct user_record *users;
};

/**
 * Creates a new simplescim_user_list object.
 * On success, a pointer to the new object is returned.
 * On error, NULL is returned and simplescim_error_string
 * is set to an appropriate error message.
 */
struct simplescim_user_list *simplescim_user_list_new()
{
	struct simplescim_user_list *this;

	this = malloc(sizeof(struct simplescim_user_list));

	if (this == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_user_list_new:"
			"malloc"
		);
		return NULL;
	}

	this->n_users = 0;
	this->users = NULL;

	return this;
}

/**
 * Deletes 'this' and all associated dynamically allocated
 * memory.
 */
void simplescim_user_list_delete(
	struct simplescim_user_list *this
)
{
	struct user_record *s, *tmp;

	if (this != NULL) {
		if (this->users != NULL) {
			HASH_ITER(hh, this->users, s, tmp) {
				HASH_DEL(this->users, s);
				simplescim_arbval_delete(s->uid);
				simplescim_user_delete(s->user);
				free(s);
			}
		}

		free(this);
	}
}

/**
 * Returns the number of users in 'this'.
 */
size_t simplescim_user_list_get_n_users(
	const struct simplescim_user_list *this
)
{
	return this->n_users;
}

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
)
{
	struct user_record *s;

	/* Check if 'uid' is already in the hash table, in
	   which case 'user' should replace the previous
	   user associated with 'uid' in the hash table
	   rather than be inserted. */
	HASH_FIND(hh,
	          this->users,
	          uid->av_val,
	          uid->av_len,
	          s);

	if (s == NULL) {
		/* 'uid' is not in the hash table so 'user'
		   should be inserted. */
		s = malloc(sizeof(struct user_record));

		if (s == NULL) {
			simplescim_error_string_set_errno(
				"simplescim_user_list_insert_user:"
				"malloc"
			);
			return -1;
		}

		s->uid = uid;
		s->user = user;

		HASH_ADD_KEYPTR(hh,
		                this->users,
		                s->uid->av_val,
		                s->uid->av_len,
		                s);

		++this->n_users;
	} else {
		/* 'uid' is already in the hash table, so
		   'user' should replace the previous user
		   associated with 'uid'. 'uid' can be
		   freed since an identical dynamically
		   allocated object is already in the hash
		   table. */
		simplescim_arbval_delete(uid);
		simplescim_user_delete(s->user);
		s->user = user;
	}

	return 0;
}

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
)
{
	struct user_record *s;

	HASH_FIND(hh,
	          this->users,
	          uid->av_val,
	          uid->av_len,
	          s);

	if (s == NULL) {
		return -1;
	}

	if (userp != NULL) {
		*userp = s->user;
	}

	return 0;
}

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
)
{
	struct user_record *s, *tmp;
	int err;

	HASH_ITER(hh, this->users, s, tmp) {
		err = func(s->uid, s->user);

		if (err == -1) {
			return -1;
		}
	}

	return 0;
}

#include <stdio.h>

/**
 * Compares 'this' to 'cache' and performs
 * 'create_user_func' on users in 'this' but not in
 * 'cache', performs 'update_user_func' on users in both
 * 'this' and 'cache' if the user has been updated and
 * performs 'delete_user_func' on users in 'cache' but not
 * in 'this'.
 */
int simplescim_user_list_find_changes(
	const struct simplescim_user_list *this,
	const struct simplescim_user_list *cache,
	int (create_user_func)(const struct simplescim_user *user),
	int (update_user_func)(const struct simplescim_user *user,
	                       const struct simplescim_user *cached_user),
	int (delete_user_func)(const struct simplescim_user *cached_user)
)
{
	struct user_record *s, *tmp;
	const struct simplescim_user *cached_user;
	int err;
	size_t n_create = 0, n_create_fail = 0;
	size_t n_update = 0, n_update_fail = 0;
	size_t n_delete = 0, n_delete_fail = 0;

	/* For every user in 'this' */
	HASH_ITER(hh, this->users, s, tmp) {
		/* Get user from 'cache' */
		err = simplescim_user_list_get_user(
			cache,
			s->uid,
			&cached_user
		);

		if (err == -1) {
			/* User doesn't exist in 'cache',
			   create it */
			++n_create;
			err = create_user_func(s->user);

			if (err == -1) {
				++n_create_fail;
			}
		} else if (!simplescim_user_eq(s->user,
		                               cached_user)) {
			/* User exists in 'cache' but is different,
			   update it */
			++n_update;
			err = update_user_func(s->user, cached_user);

			if (err == -1) {
				++n_update_fail;
			}
		}
	}

	/* For every user in 'cache' */
	HASH_ITER(hh, cache->users, s, tmp) {
		/* Get user from 'this' */
		err = simplescim_user_list_get_user(
			this,
			s->uid,
			NULL
		);

		if (err == -1) {
			/* User doesn't exist in 'this',
			   delete it */
			++n_delete;
			err = delete_user_func(s->user);

			if (err == -1) {
				++n_delete_fail;
			}
		}
	}

	printf("Status:   Success   Failure     Total\n");
	printf("Create: %9lu %9lu %9lu\n",
	       n_create - n_create_fail,
	       n_create_fail,
	       n_create);
	printf("Update: %9lu %9lu %9lu\n",
	       n_update - n_update_fail,
	       n_update_fail,
	       n_update);
	printf("Delete: %9lu %9lu %9lu\n",
	       n_delete - n_delete_fail,
	       n_delete_fail,
	       n_delete);

	return 0;
}
