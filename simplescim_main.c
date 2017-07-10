#include <glib.h>

#include "simplescim_globals.h"
#include "simplescim_config_file_parser.h"
#include "simplescim_config_file_required_variables.h"
#include "simplescim_ldap_session.h"

int main(int argc, char *argv[])
{
	int i;

	for (i = 1; i < argc; ++i) {
		/* Get config file name */
		simplescim_global_filename = argv[i];

		/* Parse config file to hash table */
		simplescim_global_vars = simplescim_parse_config_file();

		if (simplescim_global_vars == NULL) {
			simplescim_globals_reset();
			continue;
		}

		/* Connect to LDAP server */
		if (simplescim_ldap_session_start() == -1) {
			simplescim_globals_reset();
			continue;
		}

		/* Get user account data */
		if (simplescim_ldap_session_search() == -1) {
			simplescim_ldap_session_destroy_result();
			simplescim_ldap_session_close();
			simplescim_globals_reset();
			continue;
		}

		/* Print user account data */
		if (simplescim_ldap_session_print_result() == -1) {
			simplescim_ldap_session_destroy_result();
			simplescim_ldap_session_close();
			simplescim_globals_reset();
			continue;
		}

		/* Clean up LDAP session */
		simplescim_ldap_session_destroy_result();
		simplescim_ldap_session_close();

		/* Clean up global variables */
		simplescim_globals_reset();
	}

	return 0;
}
