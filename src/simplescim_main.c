#include <stdio.h>

#include "simplescim_error_string.h"
#include "simplescim_user_list.h"
#include "simplescim_config_file.h"
#include "simplescim_ldap.h"
#include "simplescim_cache_file.h"
#include "simplescim_scim.h"

static void print_attribute(const char *attribute,
                            const struct berval **values)
{
	size_t i;

	for (i = 0; values[i] != NULL; ++i) {
		printf("%s: %s\n", attribute, values[i]->bv_val);
	}

	printf("\n");
}

static void print_user(const struct berval *unique_identifier,
                       const struct simplescim_user *user)
{
	printf("====================\n");
	printf("=       USER       =\n");
	printf("====================\n\n");
	printf("unique identifier: \"%s\"\n\n", unique_identifier->bv_val);
	simplescim_user_foreach(user, print_attribute);
}

static void print_user_list(struct simplescim_user_list *users)
{
	simplescim_user_list_foreach(users, print_user);
}

int main(int argc, char *argv[])
{
	int i;
	int err;
	struct simplescim_user_list *current/*, *cached*/;

	for (i = 1; i < argc; ++i) {
		/* Read configuration file */
		err = simplescim_config_file_read(argv[i]);

		if (err == -1) {
			fprintf(stderr,
			        "%s\n",
			        simplescim_error_string);
			continue;
		}

		/* Read LDAP catalogue */
		current = simplescim_ldap_read();

		if (current == NULL) {
			fprintf(stderr,
			        "%s\n",
			        simplescim_error_string);
			simplescim_config_file_clear();
			continue;
		}

		print_user_list(current);

		/* Read cache */
		/*cached = simplescim_cache_file_load();

		if (cached == NULL) {
			fprintf(stderr,
			        "%s\n",
			        simplescim_error_string);
			simplescim_user_list_delete(current);
			simplescim_config_file_clear();
			continue;
		}*/

		/* Perform SCIM requests */
		/*err = simplescim_scim_perform(current, cached);

		if (err == -1) {
			fprintf(stderr,
			        "%s\n",
			        simplescim_error_string);
			simplescim_user_list_delete(cached);
			simplescim_user_list_delete(current);
			simplescim_config_file_clear();
			continue;
		}*/

		/*simplescim_user_list_delete(cached);*/
		simplescim_user_list_delete(current);
		simplescim_config_file_clear();
	}

	return 0;
}
