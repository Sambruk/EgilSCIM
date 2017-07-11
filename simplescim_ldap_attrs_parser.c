#include "simplescim_ldap_attrs_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "simplescim_globals.h"

static struct {
	const char *inp;
	const char *cur;
	size_t col;
	char **attrs;
	size_t n_attrs;
} parser;

static void parser_reset()
{
	parser.inp = NULL;
	parser.cur = NULL;
	parser.col = 0;
	parser.attrs = NULL;
	parser.n_attrs = 0;
}

static void syntax_error(const char *str)
{
	fprintf(stderr,
	        "%s:%s:%lu: syntax error: %s\n",
	        simplescim_global_filename,
	        "attrs",
	        parser.col,
	        str);
}

static void syntax_error_expected(const char *str)
{
	fprintf(stderr,
	        "%s:%s:%lu: syntax error: expected %s, found ",
	        simplescim_global_filename,
	        "attrs",
	        parser.col,
	        str);

	if (isgraph(*parser.cur)) {
		fprintf(stderr, "'%c'\n", *parser.cur);
	} else {
		fprintf(stderr, "0x%02X\n", *parser.cur);
	}
}

static void skip_ws()
{
	while (*parser.cur == ' ' || *parser.cur == '\t') {
		++parser.cur;
		++parser.col;
	}
}

static int parse_attr(char **dest)
{
	size_t attr_len = 0;
	char *attr;

	skip_ws();

	while (parser.cur[attr_len] != ' '
	       && parser.cur[attr_len] != '\t'
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
		fprintf(stderr,
		        "%s:%s:%s: %s\n",
		        simplescim_global_filename,
		        "attrs",
		        "malloc",
		        strerror(errno));
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
 * Parses a string containing comma separated attribute names into a
 * NULL terminated list of strings. If attrs is empty, *dest is set
 * to NULL.
 *
 * On success, zero is returned. On error, -1 is returned and an
 * error message is printed to stderr.
 */
int simplescim_parse_ldap_attrs(const char *attrs, char ***dest)
{
	size_t i;

	parser.inp = attrs;
	parser.cur = attrs;
	parser.col = 1;
	parser.n_attrs = 1;

	/* If no attributes are specified, set *dest to NULL */
	if (strcmp(attrs, "") == 0) {
		*dest = NULL;
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
		fprintf(stderr,
		        "%s:%s:%s: %s\n",
		        simplescim_global_filename,
		        "attrs",
		        "malloc",
		        strerror(errno));
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
				parser_reset();

				return -1;
			}

			++parser.cur;
			++parser.col;
		}

		if (parse_attr(&parser.attrs[i]) == -1) {
			while (i > 0) {
				free(parser.attrs[i - 1]);
				--i;
			}

			free(parser.attrs);
			parser_reset();

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
		parser_reset();

		return -1;
	}

	parser.attrs[parser.n_attrs] = NULL;
	*dest = parser.attrs;
	parser_reset();

	return 0;
}
