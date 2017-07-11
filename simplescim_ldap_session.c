#include "simplescim_ldap_session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <glib.h>
#include <ldap.h>

#include "simplescim_globals.h"
#include "simplescim_ldap_attrs_parser.h"

static LDAP *ld = NULL;
static LDAPMessage *res = NULL;

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
		ldap_print_error(err, "ldap_sasl_bind_s");
		ldap_destroy(ld);
		ld = NULL;
		return -1;
	}

	return 0;
}

int simplescim_ldap_session_search()
{
	const char *base, *scope, *filter, *attrs, *attrsonly;
	char **attrs_val;
	int scope_val = -1, attrsonly_val;
	size_t i;
	int err;

	/* Fetch variables from parsed config file */
	base = g_hash_table_lookup(simplescim_global_vars,
	                           "ldap-base");
	scope = g_hash_table_lookup(simplescim_global_vars,
	                            "ldap-scope");
	filter = g_hash_table_lookup(simplescim_global_vars,
	                             "ldap-filter");
	attrs = g_hash_table_lookup(simplescim_global_vars,
	                            "ldap-attrs");
	attrsonly = g_hash_table_lookup(simplescim_global_vars,
	                                "ldap-attrsonly");

	/* Set search scope */
	if (strcmp(scope, "BASE") == 0) {
		scope_val = LDAP_SCOPE_BASE;
	} else if (strcmp(scope, "ONELEVEL") == 0) {
		scope_val = LDAP_SCOPE_ONELEVEL;
	} else if (strcmp(scope, "SUBTREE") == 0) {
		scope_val = LDAP_SCOPE_SUBTREE;
	} else {
		scope_val = LDAP_SCOPE_CHILDREN;
	}

	/* Parse attrs */
	if (simplescim_parse_ldap_attrs(attrs, &attrs_val) == -1) {
		return -1;
	}

	/* Set attrsonly */
	if (strcmp(attrsonly, "TRUE") == 0) {
		attrsonly_val = 1;
	} else {
		attrsonly_val = 0;
	}

	err = ldap_search_ext_s(
		ld,
		base,
		scope_val,
		filter,
		attrs_val,
		attrsonly_val,
		NULL,
		NULL,
		NULL,
		-1,
		&res
	);

	if (attrs_val != NULL) {
		for (i = 0; attrs_val[i] != NULL; ++i) {
			free(attrs_val[i]);
		}

		free(attrs_val);
	}

	if (err != LDAP_SUCCESS) {
		ldap_print_error(err, "ldap_search_ext_s");
		ldap_destroy(ld);
		ld = NULL;
		return -1;
	}

	return 0;
}

int print_result(LDAPMessage *result)
{
	int err;
	int errcode;
	char *matcheddn;
	char *errmsg;
	char **referrals;

	err = ldap_parse_result(
		ld,
		result,
		&errcode,
		&matcheddn,
		&errmsg,
		&referrals,
		NULL,
		0
	);

	if (err != LDAP_SUCCESS) {
		ldap_print_error(err, "ldap_parse_result");
		return -1;
	}

	printf("result: %d %s\n", errcode, ldap_err2string(errcode));

	if (matcheddn != NULL) {
		if (matcheddn[0] != '\0') {
			printf("Matched DN: %s\n", matcheddn);
		}

		ber_memfree(matcheddn);
	}

	if (errmsg != NULL) {
		if (errmsg[0] != '\0') {
			printf("Additional information: %s\n", errmsg);
		}

		ber_memfree(errmsg);
	}

	if (referrals != NULL) {
		size_t i;

		for (i = 0; referrals[i] != NULL; ++i) {
			printf("Referral: %s\n", referrals[i]);
		}

		ber_memvfree((void **)referrals);
	}

	return 0;
}

int simplescim_ldap_session_print_result()
{
	LDAPMessage *msg;

	for (msg = ldap_first_message(ld, res);
	     msg != NULL;
	     msg = ldap_next_message(ld, msg)) {
		switch (ldap_msgtype(msg)) {
		case LDAP_RES_SEARCH_ENTRY:
			printf("LDAP_RES_SEARCH_ENTRY\n");
			/*print_entry(msg);*/
			break;

		case LDAP_RES_SEARCH_REFERENCE:
			printf("LDAP_RES_SEARCH_REFERENCE\n");
			/*print_reference(msg);*/
			break;

		case LDAP_RES_EXTENDED:
			printf("LDAP_RES_EXTENDED\n");
			/*print_extended(msg);*/
			break;

		case LDAP_RES_SEARCH_RESULT:
			printf("LDAP_RES_SEARCH_RESULT\n");

			if (print_result(msg) == -1) {
				return -1;
			}

			break;

		case LDAP_RES_INTERMEDIATE:
			printf("LDAP_RES_INTERMEDIATE\n");
			break;
		}
	}

	return 0;
}

int simplescim_ldap_session_destroy_result()
{
	ldap_msgfree(res);
	res = NULL;
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
