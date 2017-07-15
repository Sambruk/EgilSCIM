#include "simplescim_user_list.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <lber.h>
#include "uthash.h"

#include "simplescim_error_string.h"
#include "simplescim_config_file.h"
#include "simplescim_user.h"

/**
 * The uthash record for the user list object.
 */
struct user_record {
	struct berval *unique_identifier;
	struct simplescim_user *user;
	UT_hash_handle hh;
};

/**
 * The user list object. It is only a wrapper around the
 * above uthash record.
 */
struct simplescim_user_list {
	struct user_record *users;
};

/**
 * Creates a new user list object.
 * On success, a pointer to the new object is returned. On
 * error, NULL is returned and 'simplescim_error_string' is
 * set to an appropriate error message.
 */
struct simplescim_user_list *simplescim_user_list_new()
{
	struct simplescim_user_list *this;

	this = malloc(sizeof(struct simplescim_user_list));

	if (this == NULL) {
		sprintf(simplescim_error_string,
		        "%s:simplescim_user_list_new: %s",
		        simplescim_config_file_name,
		        strerror(errno));
		return NULL;
	}

	this->users = NULL;

	return this;
}

/**
 * Deletes a user list object and all dynamically allocated
 * memory associated with it.
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
 * Associates 'user' with 'unique_identifier'.
 * 'unique_identifier' is a dynamically allocated
 * 'struct berval' and 'user' is a dynamically allocated
 * user object.
 * On success, zero is returned. On error, -1 is
 * returned and 'simplescim_error_string' is set to an
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
		   table so 'users' should be inserted. */
		s = malloc(sizeof(struct user_record));

		if (s == NULL) {
			sprintf(simplescim_error_string,
"%s:simplescim_user_list_set_attribute: %s",
			        simplescim_config_file_name,
			        strerror(errno));
			return -1;
		}

		s->unique_identifier = unique_identifier;
		s->user = user;

		HASH_ADD_KEYPTR(hh,
		                this->users,
		                s->unique_identifier->bv_val,
		                s->unique_identifier->bv_len,
		                s);

		return 0;
	}

	/* 'unique_identifier' is already in the hash
	   table, so 'users' should replace the previous
	   user associated with 'unique_identifier'.
	   'unique_identifier' can be freed since an
	   identical dynamically allocated object is
	   already in the hash table. */
	free(unique_identifier->bv_val);
	free(unique_identifier);
	simplescim_user_delete(s->user);
	s->user = user;

	return 0;
}

/**
 * Gets the user associated with 'unique_identifier' and
 * stores it in 'userp'. 'userp' is a pointer to a user
 * object.
 * If 'unique_identifier' has an associated user, zero is
 * returned. Otherwise, -1 is returned and 'userp' remains
 * untouched.
 */
int
simplescim_user_list_get_user(struct simplescim_user_list *this,
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

	*userp = s->user;

	return 0;
}

/**
 * Perform 'func' for every user in the user list.
 * 'func' must have the following definition:
 * void func(const struct berval *unique_identifier,
 *           const struct simplescim_user *user);
 */
void simplescim_user_list_foreach(
	struct simplescim_user_list *this,
	void (*func)(const struct berval *unique_identifier,
	             const struct simplescim_user *user)
)
{
	struct user_record *s, *tmp;

	HASH_ITER(hh, this->users, s, tmp) {
		func(s->unique_identifier, s->user);
	}
}
