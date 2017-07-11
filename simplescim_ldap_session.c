#include "simplescim_ldap_session.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
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

static void print_berval(struct berval *bv)
{
	size_t i;

	for (i = 0; i < bv->bv_len; ++i) {
		if (isgraph(bv->bv_val[i])) {
			putchar(bv->bv_val[i]);
		} else {
			printf("\\x%02X", bv->bv_val[i]);
		}
	}
}

static int print_entry(LDAPMessage *entry)
{
	char *attr;
	struct berval **vals;
	BerElement *ber;
	size_t i;

	for (attr = ldap_first_attribute(ld, entry, &ber);
	     attr != NULL;
	     attr = ldap_next_attribute(ld, entry, ber)) {
		vals = ldap_get_values_len(ld, entry, attr);

		if (vals == NULL) {
			continue;
		}

		for (i = 0; vals[i] != NULL; ++i) {
			printf("%s: ", attr);
			print_berval(vals[i]);
			printf("\n");
		}

		ldap_value_free_len(vals);
	}

	return 0;
}

int simplescim_ldap_session_print_result()
{
	LDAPMessage *entry;

	for (entry = ldap_first_entry(ld, res);
	     entry != NULL;
	     entry = ldap_next_entry(ld, entry)) {
		if (print_entry(entry) == -1) {
			return -1;
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
