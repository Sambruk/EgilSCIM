#include "simplescim_ldap_attrs_parser.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "simplescim_error_string.h"
#include "simplescim_config_file.h"

/**
 * A global static data structure containing the current
 * state and position of the parser.
 */
static struct {
	const char *cur;
	size_t line;
	size_t col;
	char **attrs;
	size_t n_attrs;
} parser;

/**
 * Resets the parser state.
 */
static void reset_parser()
{
	parser.cur = NULL;
	parser.line = 0;
	parser.col = 0;
	parser.attrs = NULL;
	parser.n_attrs = 0;
}

/**
 * Prints a syntax error to simplescim_error_string
 * according to global static 'parser' object and 'str'.
 */
static void syntax_error(const char *str)
{
	simplescim_error_string_set_prefix(
		"%s:ldap-attrs:%lu:%lu:syntax error",
		simplescim_config_file_name,
		parser.line,
		parser.col
	);
	simplescim_error_string_set_message(
		"%s",
		str
	);
}

/**
 * Prints a syntax error to 'simplescim_error_string'
 * according to global static 'parser' object and 'str',
 * when the error is of type "expected x, found y".
 */
static void syntax_error_expected(const char *str)
{
	simplescim_error_string_set_prefix(
		"%s:ldap-attrs:%lu:%lu:syntax error",
		simplescim_config_file_name,
		parser.line,
		parser.col
	);

	if (isprint(*parser.cur)) {
		simplescim_error_string_set_message(
			"expected %s, found '%c'",
			str,
			*parser.cur
		);
	} else {
		simplescim_error_string_set_message(
			"expected %s, found 0x%02X",
			str,
			*parser.cur
		);
	}
}

static void skip_ws()
{
	while (*parser.cur == ' '
	       || *parser.cur == '\t'
	       || *parser.cur == '\n') {
		if (*parser.cur == '\n') {
			++parser.cur;
			++parser.line;
			parser.col = 1;
		} else {
			++parser.cur;
			++parser.col;
		}
	}
}

static int parse_attr(char **dest)
{
	size_t attr_len = 0;
	char *attr;

	skip_ws();

	while (parser.cur[attr_len] != ' '
	       && parser.cur[attr_len] != '\t'
	       && parser.cur[attr_len] != '\n'
	       && parser.cur[attr_len] != ','
	       && parser.cur[attr_len] != '\0') {
		++attr_len;
	}

	if (attr_len == 0) {
		syntax_error("empty attribute name");
		return -1;
	}

	attr = malloc(attr_len + 1);

	if (attr == NULL) {
		simplescim_error_string_set_errno(
			"parse_attr:"
			"malloc"
		);
		return -1;
	}

	memcpy(attr, parser.cur, attr_len);
	attr[attr_len] = '\0';
	*dest = attr;

	parser.cur += attr_len;
	parser.col += attr_len;

	skip_ws();

	return 0;
}

/**
 * Parses a comma separated list of attributes into a
 * NULL-terminated list of strings.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_ldap_attrs_parser(const char *attrs, char ***attrs_val)
{
	size_t i;
	int err;

	parser.cur = attrs;
	parser.line = 1;
	parser.col = 1;
	parser.n_attrs = 1;

	/* If no attributes are specified, set *dest to NULL */
	if (strcmp(attrs, "") == 0) {
		*attrs_val = NULL;
		return 0;
	}

	/* Find number of attributes */
	for (i = 0; attrs[i] != '\0'; ++i) {
		if (attrs[i] == ',') {
			++parser.n_attrs;
		}
	}

	/* Allocate attrs */
	parser.attrs = malloc(sizeof(char *) * (parser.n_attrs + 1));

	if (parser.attrs == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_ldap_attrs_parser:"
			"malloc"
		);
		reset_parser();
		return -1;
	}

	/* Parse all attributes */
	for (i = 0; i < parser.n_attrs; ++i) {
		if (i > 0) {
			if (*parser.cur != ',') {
				syntax_error_expected("','");

				while (i > 0) {
					free(parser.attrs[i - 1]);
					--i;
				}

				free(parser.attrs);
				reset_parser();

				return -1;
			}

			++parser.cur;
			++parser.col;
		}

		err = parse_attr(&parser.attrs[i]);

		if (err == -1) {
			while (i > 0) {
				free(parser.attrs[i - 1]);
				--i;
			}

			free(parser.attrs);
			reset_parser();

			return -1;
		}
	}

	if (*parser.cur != '\0') {
		syntax_error_expected("end-of-string");

		while (i > 0) {
			free(parser.attrs[i - 1]);
			--i;
		}

		free(parser.attrs);
		reset_parser();

		return -1;
	}

	parser.attrs[parser.n_attrs] = NULL;
	*attrs_val = parser.attrs;
	reset_parser();

	return 0;
}
