#include <stdio.h>

#include "simplescim_error_string.h"
#include "simplescim_user_list.h"
#include "simplescim_config_file.h"
#include "simplescim_ldap.h"
#include "simplescim_cache_file.h"
#include "simplescim_scim.h"

static void print_error()
{
	fprintf(stderr, "%s\n", simplescim_error_string_get());
}

static void print_status(const char *config_file_name)
{
	fprintf(stdout,
	        "Successfully performed SCIM operations for %s\n",
	        config_file_name);
}

int main(int argc, char *argv[])
{
	struct simplescim_user_list *ldap, *cache;
	int i;
	int err;

	for (i = 1; i < argc; ++i) {
		/* Load configuration file */
		err = simplescim_config_file_load(argv[i]);

		if (err == -1) {
			print_error();
			continue;
		}

		/* Get users from LDAP catalogue */
		ldap = simplescim_ldap_get_users();

		if (ldap == NULL) {
			print_error();
			simplescim_config_file_clear();
			continue;
		}

		/* Get users from cache file */
		cache = simplescim_cache_file_get_users();

		if (cache == NULL) {
			print_error();
			simplescim_user_list_delete(ldap);
			simplescim_config_file_clear();
			continue;
		}

		/* Perform SCIM operations */
		err = simplescim_scim_perform(ldap, cache);

		if (err == -1) {
			print_error();
			simplescim_user_list_delete(cache);
			simplescim_user_list_delete(ldap);
			simplescim_config_file_clear();
			continue;
		}

		print_status(argv[i]);

		/* Clean up */
		simplescim_user_list_delete(cache);
		simplescim_user_list_delete(ldap);
		simplescim_config_file_clear();
	}

	return 0;
}
