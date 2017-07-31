#include "simplescim_user_list.h"

#include <stdlib.h>
#include <lber.h>
#include "uthash.h"

#include "simplescim_error_string.h"
#include "simplescim_user.h"

struct user_record {
	struct berval *unique_identifier;
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
			"simplescim_user_list_new:malloc"
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
void simplescim_user_list_delete(struct simplescim_user_list *this)
{
	struct user_record *s, *tmp;

	HASH_ITER(hh, this->users, s, tmp) {
		HASH_DEL(this->users, s);
		free(s->unique_identifier->bv_val);
		free(s->unique_identifier);
		simplescim_user_delete(s->user);
		free(s);
	}

	free(this);
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
 * Associates 'unique_identifier' with 'user' in 'this'.
 * 'unique_identifier' is a dynamically allocated
 * struct berval object and 'user' is a dynamically
 * allocated simplescim_user object.
 * On success, zero is returned. On error, -1 is
 * returned and simplescim_error_string is set to an
 * appropriate error message.
 */
int simplescim_user_list_insert_user(struct simplescim_user_list *this,
                                     struct berval *unique_identifier,
                                     struct simplescim_user *user)
{
	struct user_record *s;

	/* Check if 'unique_identifier' is already in the
	   hash table, in which case 'user' should replace
	   the previous user associated with
	   'unique_identifier' in the hash table rather
	   than be inserted. */
	HASH_FIND(hh,
	          this->users,
	          unique_identifier->bv_val,
	          unique_identifier->bv_len,
	          s);

	if (s == NULL) {
		/* 'unique_identifier' is not in the hash
		   table so 'user' should be inserted. */
		s = malloc(sizeof(struct user_record));

		if (s == NULL) {
			simplescim_error_string_set_errno(
				"simplescim_user_list_insert_user:malloc"
			);
			return -1;
		}

		s->unique_identifier = unique_identifier;
		s->user = user;

		HASH_ADD_KEYPTR(hh,
		                this->users,
		                s->unique_identifier->bv_val,
		                s->unique_identifier->bv_len,
		                s);

		++this->n_users;
	} else {
		/* 'unique_identifier' is already in the hash
		   table, so 'user' should replace the previous
		   user associated with 'unique_identifier'.
		   'unique_identifier' can be freed since an
		   identical dynamically allocated object is
		   already in the hash table. */
		free(unique_identifier->bv_val);
		free(unique_identifier);
		simplescim_user_delete(s->user);
		s->user = user;
	}

	return 0;
}

/**
 * Gets the user associated with 'unique_identifier' in
 * 'this' and stores it in 'userp'.
 * If 'unique_identifier' has an associated user, zero is
 * returned. Otherwise, -1 is returned.
 */
int simplescim_user_list_get_user(const struct simplescim_user_list *this,
                                  const struct berval *unique_identifier,
                                  const struct simplescim_user **userp)
{
	struct user_record *s;

	HASH_FIND(hh,
	          this->users,
	          unique_identifier->bv_val,
	          unique_identifier->bv_len,
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
 * void func(const struct berval *unique_identifier,
 *           const struct simplescim_user *user);
 */
void simplescim_user_list_foreach(
	const struct simplescim_user_list *this,
	void (*func)(const struct berval *unique_identifier,
	             const struct simplescim_user *user)
)
{
	struct user_record *s, *tmp;

	HASH_ITER(hh, this->users, s, tmp) {
		func(s->unique_identifier, s->user);
	}
}

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

	/* For every user in 'this' */
	HASH_ITER(hh, this->users, s, tmp) {
		/* Get user from 'cache' */
		err = simplescim_user_list_get_user(
			cache,
			s->unique_identifier,
			&cached_user
		);

		if (err == -1) {
			/* User doesn't exist in 'cache',
			   create it */
			err = create_user_func(s->user);

			if (err == -1) {
				return -1;
			}
		} else if (!simplescim_user_subset_eq(s->user,
		                                      cached_user)) {
			/* User exists in 'cache' but is different,
			   update it */
			err = update_user_func(s->user, cached_user);

			if (err == -1) {
				return -1;
			}
		}
	}

	/* For every user in 'cache' */
	HASH_ITER(hh, cache->users, s, tmp) {
		/* Get user from 'this' */
		err = simplescim_user_list_get_user(
			this,
			s->unique_identifier,
			NULL
		);

		if (err == -1) {
			/* User doesn't exist in 'this',
			   delete it */
			err = delete_user_func(s->user);

			if (err == -1) {
				return -1;
			}
		}
	}

	return 0;
}
