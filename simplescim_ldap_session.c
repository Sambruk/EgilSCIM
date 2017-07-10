#include "simplescim_ldap_session.h"

#include <stdio.h>

#include "simplescim_globals.h"

int simplescim_ldap_session_start()
{
	return 0;
}

int simplescim_ldap_session_search()
{
	return 0;
}

int simplescim_ldap_session_print_result()
{
	printf("Printing LDAP output from %s\n",
	       simplescim_global_filename);
	return 0;
}

int simplescim_ldap_session_destroy_result()
{
	return 0;
}

int simplescim_ldap_session_close()
{
	return 0;
}
