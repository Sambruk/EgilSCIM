#include <stdio.h>
#include <stdlib.h>

#include "simplescim_error_string.h"
#include "simplescim_arbval.h"
#include "simplescim_arbval_list.h"
#include "simplescim_user.h"
#include "simplescim_user_list.h"
#include "simplescim_cache_file.h"

static int print_attr(
	const char *attribute,
	const struct simplescim_arbval_list *values
)
{
	size_t i;

	for (i = 0; i < values->al_len; ++i) {
		printf("%s: %s\n", attribute, values->al_vals[i]->av_val);
	}

	return 0;
}

static int print_user(
	const struct simplescim_arbval *uid,
	const struct simplescim_user *user
)
{
	printf("==== User ====\n");
	printf("== Unique identifier: %s\n", uid->av_val);
	simplescim_user_foreach(
		user,
		&print_attr
	);
	return 0;
}

int main(int argc, char *argv[])
{
	struct simplescim_user_list *users;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s cache-file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	users = simplescim_cache_file_get_users_from_file(argv[1]);

	if (users == NULL) {
		fprintf(stderr, "%s\n", simplescim_error_string_get());
		exit(EXIT_FAILURE);
	}

	simplescim_user_list_foreach(
		users,
		&print_user
	);

	simplescim_user_list_delete(users);

	return 0;
}
