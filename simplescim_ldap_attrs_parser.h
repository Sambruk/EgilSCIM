#ifndef SIMPLESCIM_LDAP_ATTRS_PARSER_H
#define SIMPLESCIM_LDAP_ATTRS_PARSER_H

/**
 * Parses a string containing comma separated attribute names into a
 * NULL terminated list of strings. If attrs is empty, *dest is set
 * to NULL.
 *
 * On success, zero is returned. On error, -1 is returned and an
 * error message is printed to stderr.
 */
int simplescim_parse_ldap_attrs(const char *attrs, char ***dest);

#endif
