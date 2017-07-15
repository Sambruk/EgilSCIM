#ifndef SIMPLESCIM_LDAP_ATTRS_PARSER_H
#define SIMPLESCIM_LDAP_ATTRS_PARSER_H

/**
 * Parses a comma separated list of attributes into a
 * NULL-terminated list of strings.
 * On success, zero is returned. On error, -1 is returned
 * and 'simplescim_error_string' is set to an appropriate
 * error message.
 */
int simplescim_ldap_attrs_parser(const char *attrs, char ***attrs_val);

#endif
