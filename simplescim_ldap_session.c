#include "simplescim_ldap_session.h"

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <ldap.h>

#include "simplescim_globals.h"

static LDAP *ld = NULL;

static void ldap_print_error(int err, const char *func)
{
	fprintf(stderr,
	        "%s:%s: %s\n",
	        simplescim_global_filename,
	        func,
	        ldap_err2string(err));
}

int simplescim_ldap_session_start()
{
	const char *uri, *who, *passwd;
	int ldap_version = LDAP_VERSION3;
	struct berval cred;
	int err;

	uri = g_hash_table_lookup(simplescim_global_vars, "ldap-uri");
	who = g_hash_table_lookup(simplescim_global_vars, "ldap-who");
	passwd = g_hash_table_lookup(simplescim_global_vars, "ldap-passwd");

	err = ldap_initialize(&ld, uri);

	if (err != LDAP_SUCCESS) {
		ldap_print_error(err, "ldap_initialize");
		return -1;
	}

	err = ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &ldap_version);

	if (err != LDAP_OPT_SUCCESS) {
		ldap_print_error(err, "ldap_set_option");
		ldap_destroy(ld);
		ld = NULL;
		return -1;
	}

	cred.bv_val = (char *)passwd;
	cred.bv_len = strlen(passwd);

	err = ldap_sasl_bind_s(ld, who, LDAP_SASL_SIMPLE, &cred,
	                       NULL, NULL, NULL);

	if (err != LDAP_SUCCESS) {
		ldap_print_error(err, "ldap_simple_bind_s");
		ldap_destroy(ld);
		ld = NULL;
		return -1;
	}

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
	int err;

	err = ldap_unbind_ext(ld, NULL, NULL);

	if (err != LDAP_SUCCESS) {
		ldap_print_error(err, "ldap_unbind_ext");
		ldap_destroy(ld);
		ld = NULL;
		return -1;
	}

	ld = NULL;

	return 0;
}
