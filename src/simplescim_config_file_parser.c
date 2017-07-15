#include "simplescim_config_file_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

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
} parser;

/**
 * Resets the parser state.
 */
static void reset_parser()
{
	parser.cur = NULL;
	parser.line = 0;
	parser.col = 0;
}

/**
 * Prints a syntax error to 'simplescim_error_string'
 * according to global static 'parser' object and 'str'.
 */
static void syntax_error(const char *str)
{
	sprintf(simplescim_error_string,
	        "%s:%lu:%lu:syntax error: %s",
	        simplescim_config_file_name,
	        parser.line,
	        parser.col,
	        str);
}

/**
 * Prints a syntax error to 'simplescim_error_string'
 * according to global static 'parser' object and 'str',
 * when the error is of type "expected x, found y".
 */
static void syntax_error_expected(const char *str)
{
	int offset;

	offset = sprintf(simplescim_error_string,
	                 "%s:%lu:%lu:syntax error: expected %s, found ",
	                 simplescim_config_file_name,
	                 parser.line,
	                 parser.col,
	                 str);

	if (isprint(*parser.cur)) {
		sprintf(simplescim_error_string + offset,
		        "'%c'",
		        *parser.cur);
	} else {
		sprintf(simplescim_error_string + offset,
		        "0x%02X",
		        *parser.cur);
	}
}

/**
 * Returns 1 if 'c' is a valid character in a varid.
 * Returns 0 otherwise.
 */
static int is_varid(char c)
{
	static unsigned char lookup_table[0x100] = {
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 00-0F */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 10-1F */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, /* 20-2F */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, /* 30-3F */
 		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 40-4F */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 1, /* 50-5F */
		0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, /* 60-6F */
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, /* 70-7F */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 80-8F */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 90-9F */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* A0-AF */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* B0-BF */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* C0-CF */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* D0-DF */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* E0-EF */
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0  /* F0-FF */
	};

	return (int)lookup_table[(unsigned char)c];
}

/* <ws> ::= ' ' | '\t' */
static void rule_skip_ws()
{
	while (*parser.cur == ' ' || *parser.cur == '\t') {
		++parser.cur;
		++parser.col;
	}
}

/* <varid> ::= [-_a-zA-Z0-9]+ */
static int rule_varid(char **var)
{
	size_t var_len = 0;
	char *tmp;

	/* Determine variable name length */
	while (is_varid(parser.cur[var_len])) {
		++var_len;
	}

	if (var_len == 0) {
		syntax_error("empty variable name");
		return -1;
	}

	/* Allocate and copy variable name string */
	tmp = malloc(var_len + 1);

	if (tmp == NULL) {
		sprintf(simplescim_error_string,
		        "%s: %s",
		        simplescim_config_file_name,
		        strerror(errno));
		return -1;
	}

	memcpy(tmp, parser.cur, var_len);
	tmp[var_len] = '\0';
	*var = tmp;

	parser.cur += var_len;
	parser.col += var_len;

	return 0;
}

/**
 * <value> ::= '<?' [^('?>')]* '?>' <ws>*
 *           | [^('#'|'\n')]*                    # remove trailing <ws>*
 */
static int rule_value(char **val)
{
	size_t val_len = 0;
	char *tmp;

	/* Multi line value or single line value */
	if (parser.cur[0] == '<' && parser.cur[1] == '?') {
		size_t tmp_line, tmp_col;

		parser.cur += 2;
		parser.col += 2;

		tmp_line = parser.line;
		tmp_col = parser.col;

		/* Determine length of multi line value */
		for (;;) {
			if (parser.cur[val_len] == '\0') {
				parser.line = tmp_line;
				parser.col = tmp_col;
				syntax_error("unexpected end-of-file");
				return -1;
			}

			/* Multi line value terminated by '?>' */
			if (parser.cur[val_len] == '?'
			    && parser.cur[val_len + 1] == '>') {
				break;
			}

			if (parser.cur[val_len] == '\n') {
				++val_len;
				++tmp_line;
				tmp_col = 1;
			} else {
				++val_len;
				++tmp_col;
			}
		}

		/* Allocate and copy value string */
		tmp = malloc(val_len + 1);

		if (tmp == NULL) {
			sprintf(simplescim_error_string,
			        "%s: %s",
			        simplescim_config_file_name,
			        strerror(errno));
			return -1;
		}

		memcpy(tmp, parser.cur, val_len);
		tmp[val_len] = '\0';

		parser.cur += val_len + 2;
		parser.line = tmp_line;
		parser.col = tmp_col + 2;

		rule_skip_ws();
	} else {
		/* Determine single line value length */
		while (parser.cur[val_len] != '\n'
		       && parser.cur[val_len] != '#'
		       && parser.cur[val_len] != '\0') {
			++val_len;
		}

		if (parser.cur[val_len] == '\0') {
			parser.col += val_len;
			syntax_error("unexpected end-of-file");
			return -1;
		}

		/* Allocate and copy value string */
		tmp = malloc(val_len + 1);

		if (tmp == NULL) {
			sprintf(simplescim_error_string,
			        "%s: %s",
			        simplescim_config_file_name,
			        strerror(errno));
			return -1;
		}

		memcpy(tmp, parser.cur, val_len);
		tmp[val_len] = '\0';

		parser.cur += val_len;
		parser.col += val_len;

		/* Remove trailing white space */
		while (val_len > 0 && (tmp[val_len - 1] == ' '
		                       || tmp[val_len - 1] == '\t')) {
			tmp[val_len - 1] = '\0';
			--val_len;
		}
	}

	*val = tmp;

	return 0;
}

/* <assign> ::= <varid> <ws>* '=' <ws>* <value> */
static int rule_assign()
{
	char *var, *val;
	int err;

	/* Obligatory variable name */
	if (!is_varid(*parser.cur)) {
		syntax_error_expected("variable name");
		return -1;
	}

	err = rule_varid(&var);

	if (err == -1) {
		return -1;
	}

	/* Optional white space */
	rule_skip_ws();

	/* Obligatory variable assignment character */
	if (*parser.cur != '=') {
		syntax_error_expected("'='");
		free(var);
		return -1;
	}

	++parser.cur;
	++parser.col;

	/* Optional white space */
	rule_skip_ws();

	/* Obligatory value */
	err = rule_value(&val);

	if (err == -1) {
		free(var);
		return -1;
	}

	err = simplescim_config_file_insert(var, val);

	if (err == -1) {
		free(var);
		free(val);
		return -1;
	}

	return 0;
}

/* <comment> ::= '#' [^\n]* */
static int rule_comment()
{
	/* Obligatory line comment initialiser character */
	if (*parser.cur != '#') {
		syntax_error_expected("'#'");
		return -1;
	}

	++parser.cur;
	++parser.col;

	/* Zero or more non-newline characters */
	while (*parser.cur != '\n' && *parser.cur != '\0') {
		++parser.cur;
		++parser.col;
	}

	if (*parser.cur == '\0') {
		syntax_error("unexpected end-of-file");
		return -1;
	}

	return 0;
}

/* <config> ::= ( <ws>* <assign>? <comment>? '\n' )* */
static int rule_config()
{
	int err;

	/* Zero or more lines */
	while (*parser.cur != '\0') {
		/* Optional white space */
		rule_skip_ws();

		/* Optional variable assignment */
		if (is_varid(*parser.cur)) {
			err = rule_assign();

			if (err == -1) {
				return -1;
			}
		}

		/* Optional comment */
		if (*parser.cur == '#') {
			err = rule_comment();

			if (err == -1) {
				return -1;
			}
		}

		/* Obligatory newline */
		if (*parser.cur != '\n') {
			syntax_error_expected("end-of-line");
			return -1;
		}

		++parser.cur;
		++parser.line;
		parser.col = 1;
	}

	return 0;
}

/**
 * Parses 'input' into variable-value pairs and stores them
 * in the global configuration file data structure.
 * On success, zero is returned. On error, -1 is returned
 * and 'simplescim_error_string' is set to an appropriate
 * error message.
 */
int simplescim_config_file_parser(const char *input)
{
	int err;

	/* Initialise config file contents and parser
	   position variables in global data structure */
	parser.cur = input;
	parser.line = 1;
	parser.col = 1;

	/* Start parsing */
	err = rule_config();

	if (err == -1) {
		reset_parser();
		return -1;
	}

	reset_parser();

	return 0;
}
