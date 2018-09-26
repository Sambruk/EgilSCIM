/**
 * Copyright © 2017-2018  Max Wällstedt <>
 *
 * This file is part of SimpleSCIM.
 *
 * SimpleSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with SimpleSCIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "simplescim_scim_json.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "simplescim_error_string.h"
#include "simplescim_arbval.h"
#include "simplescim_arbval_list.h"
#include "simplescim_user.h"

struct simplescim_scim_json_iter {
	char *iter_var;
	const struct simplescim_arbval_list *iter_val;
	size_t iter_idx;
	struct {
		const char *json;
		size_t line;
		size_t col;
	} reset;
	struct simplescim_scim_json_iter *next;
};

struct simplescim_scim_json_parser {
	const char *json;
	const struct simplescim_user *user;
	size_t line;
	size_t col;
	struct {
		size_t len;
		size_t alloc;
		char *str;
	} output;
	struct simplescim_scim_json_iter *iteration_stack;
};

/**
 * Sets the prefix of simplescim_error_string to a syntax
 * error.
 */
static void simplescim_scim_json_parser_syntax_error(
	struct simplescim_scim_json_parser *parser
)
{
	simplescim_error_string_set_prefix(
		"simplescim_scim_json_parser:%lu:%lu:syntax error",
		parser->line,
		parser->col
	);
}

/**
 * Sets simplescim_error_string to a syntax error in the
 * format "expected x, found y".
 */
static void simplescim_scim_json_parser_syntax_error_expected(
	struct simplescim_scim_json_parser *parser,
	const char *expected
)
{
	simplescim_scim_json_parser_syntax_error(parser);

	if (isprint(parser->json[0])) {
		simplescim_error_string_set_message(
			"expected %s, found '%c'",
			expected,
			parser->json[0]
		);
	} else {
		simplescim_error_string_set_message(
			"expected %s, found 0x%02X",
			expected,
			parser->json[0]
		);
	}
}

/**
 * Creates a new simplescim_scim_json_parser object.
 * On success, a pointer to the created object is returned.
 * On error, NULL is returned and simplescim_error_string
 * is set to an appropriate error message.
 */
static struct simplescim_scim_json_parser *
simplescim_scim_json_parser_new(
	const char *json,
	const struct simplescim_user *user
)
{
	struct simplescim_scim_json_parser *parser;

	parser = malloc(sizeof(struct simplescim_scim_json_parser));

	if (parser == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_scim_json_parser_new:"
			"malloc"
		);
		return NULL;
	}

	parser->json = json;
	parser->user = user;
	parser->line = 1;
	parser->col = 1;
	parser->output.len = 0;
	parser->output.alloc = 1024;
	parser->output.str = malloc(parser->output.alloc + 1);

	if (parser->output.str == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_scim_json_parser_new:"
			"malloc"
		);
		free(parser);
		return NULL;
	}

	parser->iteration_stack = NULL;

	return parser;
}

/**
 * Deletes a simplescim_scim_json_parser object.
 */
static void simplescim_scim_json_parser_delete(
	struct simplescim_scim_json_parser *parser
)
{
	struct simplescim_scim_json_iter *iter;

	if (parser != NULL) {
		if (parser->output.str != NULL) {
			free(parser->output.str);
		}

		while (parser->iteration_stack != NULL) {
			iter = parser->iteration_stack;
			parser->iteration_stack = iter->next;
			free(iter->iter_var);
			free(iter);
		}

		free(parser);
	}
}

/**
 * Progresses the parser's input by one character.
 */
static void simplescim_scim_json_parser_progress(
	struct simplescim_scim_json_parser *parser
)
{
	if (parser->json[0] == '\n') {
		++parser->line;
		parser->col = 1;
	} else {
		++parser->col;
	}

	++parser->json;
}

/**
 * Copies the character 'c' to the output JSON string.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_scim_json_parser_copy_char(
	struct simplescim_scim_json_parser *parser,
	char c
)
{
	char *tmp;

	if (parser->output.len == parser->output.alloc) {
		parser->output.alloc *= 2;
		tmp = realloc(parser->output.str,
		              parser->output.alloc + 1);

		if (tmp == NULL) {
			simplescim_error_string_set_errno(
				"simplescim_scim_json_copy_char:"
				"realloc"
			);
			return -1;
		}

		parser->output.str = tmp;
	}

	parser->output.str[parser->output.len] = c;
	++parser->output.len;

	return 0;
}

/**
 * Progresses the parser's input past any optional white
 * space characters.
 */
static void simplescim_scim_json_parser_skip_ws(
	struct simplescim_scim_json_parser *parser
)
{
	while (isspace(parser->json[0])) {
		simplescim_scim_json_parser_progress(parser);
	}
}

/**
 * If 'c' is a character in an ID, 1 is returned.
 * Otherwise, 0 is returned.
 */
static int simplescim_scim_json_is_id(char c)
{
	if (isalnum(c) || c == '-' || c == '_') {
		return 1;
	} else {
		return 0;
	}
}

/**
 * Parses an ID in the input JSON string.
 * <id> ::= '$'? [a-zA-Z0-9\-\_]+
 * On success, a pointer to the ID is returned. On error,
 * NULL is returned and simplescim_error_string is set to
 * an appropriate error message.
 */
static char *simplescim_scim_json_parser_id(
	struct simplescim_scim_json_parser *parser
)
{
	char *id;
	size_t len, i;

	/* Check for iteration variable */
	if (parser->json[0] == '$') {
		len = 1;
	} else {
		len = 0;
	}

	/* At least one ID character */
	if (!simplescim_scim_json_is_id(parser->json[len])) {
		simplescim_scim_json_parser_syntax_error_expected(
			parser,
			"variable name or keyword"
		);
		return NULL;
	}

	/* Determine id length */
	while (simplescim_scim_json_is_id(parser->json[len])) {
		++len;
	}

	/* Allocate id */
	id = malloc(len + 1);

	if (id == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_scim_json_parser_id:"
			"malloc"
		);
		return NULL;
	}

	/* Copy id string */
	memcpy(id, parser->json, len);

	/* Terminate id string */
	id[len] = '\0';

	/* Progress the parser */
	for (i = 0; i < len; ++i) {
		simplescim_scim_json_parser_progress(parser);
	}

	return id;
}

/**
 * Writes 'val' to the output JSON string.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error string.
 */
static int simplescim_scim_json_parser_replace(
	struct simplescim_scim_json_parser *parser,
	char *val
)
{
	int err;

	while (val[0] != '\0') {
		err = simplescim_scim_json_parser_copy_char(
			parser,
			val[0]
		);

		if (err == -1) {
			return -1;
		}

		++val;
	}

	return 0;
}

/**
 * Gets the first value of the LDAP attribute or iteration
 * variable 'var'.
 * On success, a pointer to the value is returned. On
 * error, NULL is returned and simplescim_error_string is
 * set to an appropriate error message.
 */
static char *simplescim_scim_json_parser_get_val(
	struct simplescim_scim_json_parser *parser,
	char *var
)
{
	int err;
	const struct simplescim_arbval *value;
	const struct simplescim_arbval_list *values;
	struct simplescim_scim_json_iter *iter;
	char *val;

	if (var[0] == '$') {
		++var;

		/* Get iteration variable */
		iter = parser->iteration_stack;

		while (iter != NULL) {
			if (strcmp(iter->iter_var, var) == 0) {
				if (iter->iter_idx
				    >= iter->iter_val->al_len) {
					value = NULL;
				} else {
					value = iter->
					        iter_val->
					        al_vals[iter->iter_idx];
				}

				break;
			}

			iter = iter->next;
		}

		if (iter == NULL) {
			simplescim_scim_json_parser_syntax_error(
				parser
			);
			simplescim_error_string_set_message(
				"iteration variable \"%s\" does not exist",
				var
			);
			return NULL;
		}
	} else {
		/* Get LDAP variable */
		err = simplescim_user_get_attribute(
			parser->user,
			var,
			&values
		);

		if (err == -1) {
			simplescim_scim_json_parser_syntax_error(
				parser
			);
			simplescim_error_string_set_message(
				"user does not have attribute \"%s\"",
				var
			);
			return NULL;
		}

		if (values->al_len == 0) {
			value = NULL;
		} else {
			value = values->al_vals[0];
		}
	}

	/* Duplicate value */
	if (value == NULL) {
		val = strdup("");
	} else {
		val = strdup((const char *)value->av_val);
	}

	if (val == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_scim_json_parser_get_val:"
			"strdup"
		);
	}

	return val;
}

/**
 * Parses a simple replacement rule.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error string.
 */
static int simplescim_scim_json_parser_replacement_simple(
	struct simplescim_scim_json_parser *parser,
	char *var
)
{
	char *val;
	int err;

	/* Get value */
	val = simplescim_scim_json_parser_get_val(
		parser,
		var
	);

	if (val == NULL) {
		return -1;
	}

	/* Write replacement */
	err = simplescim_scim_json_parser_replace(
		parser,
		val
	);

	if (err == -1) {
		free(val);
		return -1;
	}

	free(val);

	return 0;
}

/**
 * Parses a string literal, either single or double quoted.
 * <string> ::= '\'' [^']* '\''
 *            | '"' [^"]* '"'
 * On success, a pointer to the parsed string is returned.
 * On error, NULL is returned and simplescim_error_string
 * is set to an appropriate error message.
 */
static char *simplescim_scim_json_parser_string(
	struct simplescim_scim_json_parser *parser
)
{
	char quote;
	char *str;
	size_t len;
	size_t i;

	/* Determine quote type */
	if (parser->json[0] == '\'') {
		quote = '\'';
	} else if (parser->json[0] == '"') {
		quote = '"';
	} else {
		simplescim_scim_json_parser_syntax_error_expected(
			parser,
			"single or double quote"
		);
		return NULL;
	}

	simplescim_scim_json_parser_progress(parser);

	/* Determine string length */
	len = 0;

	if (quote == '\'') {
		while (parser->json[len] != '\''
		       && parser->json[len] != '\0') {
			++len;
		}
	} else {
		while (parser->json[len] != '\"'
		       && parser->json[len] != '\0') {
			++len;
		}
	}

	if (parser->json[len] == '\0') {
		simplescim_scim_json_parser_syntax_error(parser);
		simplescim_error_string_set_message(
			"unexpected end-of-string"
		);
		return NULL;
	}

	/* Allocate and copy string */
	str = malloc(len + 1);

	if (str == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_scim_json_parser_string:"
			"malloc"
		);
		return NULL;
	}

	memcpy(str, parser->json, len);
	str[len] = '\0';

	/* Progress parser */
	for (i = 0; i <= len; ++i) {
		simplescim_scim_json_parser_progress(parser);
	}

	return str;
}

/**
 * Parses a case statement.
 * <case> ::= <ws>* <string> <ws>* ':' <ws>* <string>
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_scim_json_parser_case(
	struct simplescim_scim_json_parser *parser,
	char *val,
	int *matched
)
{
	char *match, *value;
	int err;

	simplescim_scim_json_parser_skip_ws(parser);
	match = simplescim_scim_json_parser_string(parser);

	if (match == NULL) {
		return -1;
	}

	simplescim_scim_json_parser_skip_ws(parser);

	if (parser->json[0] != ':') {
		simplescim_scim_json_parser_syntax_error_expected(
			parser,
			"':'"
		);
		free(match);
		return -1;
	}

	simplescim_scim_json_parser_progress(parser);
	simplescim_scim_json_parser_skip_ws(parser);
	value = simplescim_scim_json_parser_string(parser);

	if (value == NULL) {
		free(match);
	}

	if (*matched == 0 && strcmp(match, val) == 0) {
		err = simplescim_scim_json_parser_replace(
			parser,
			value
		);

		if (err == -1) {
			free(value);
			free(match);
			return -1;
		}

		*matched = 1;
	}

	free(value);
	free(match);

	return 0;
}

/**
 * Parses a default statement.
 * <default> ::= <ws>* ':' <ws>* <string>
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_scim_json_parser_default(
	struct simplescim_scim_json_parser *parser,
	int matched
)
{
	char *value;
	int err;

	simplescim_scim_json_parser_skip_ws(parser);

	if (parser->json[0] != ':') {
		simplescim_scim_json_parser_syntax_error_expected(
			parser,
			"':'"
		);
		return -1;
	}

	simplescim_scim_json_parser_progress(parser);
	simplescim_scim_json_parser_skip_ws(parser);
	value = simplescim_scim_json_parser_string(parser);

	if (value == NULL) {
		return -1;
	}

	if (matched == 0) {
		err = simplescim_scim_json_parser_replace(
			parser,
			value
		);

		if (err == -1) {
			free(value);
			return -1;
		}
	}

	free(value);

	return 0;
}

/**
 * Parses a conditional replacement rule.
 * <cond> ::= <ws>* <id> <ws>* ('case' <case> <ws>*)*
 *                             'default' <default> <ws>*
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error string.
 */
static int simplescim_scim_json_parser_replacement_cond(
	struct simplescim_scim_json_parser *parser
)
{
	char *id, *val;
	int matched = 0;

	simplescim_scim_json_parser_skip_ws(parser);

	/* Get variable */
	id = simplescim_scim_json_parser_id(parser);

	if (id == NULL) {
		return -1;
	}

	/* Get value */
	val = simplescim_scim_json_parser_get_val(parser, id);

	if (val == NULL) {
		free(id);
		return -1;
	}

	free(id);
	simplescim_scim_json_parser_skip_ws(parser);

	/* Iterate over all case and default statements */
	for (;;) {
		id = simplescim_scim_json_parser_id(parser);

		if (id == NULL) {
			free(val);
			return -1;
		}

		if (strcmp(id, "case") == 0) {
			simplescim_scim_json_parser_case(
				parser,
				val,
				&matched
			);
		} else if (strcmp(id, "default") == 0) {
			simplescim_scim_json_parser_default(
				parser,
				matched
			);
			break;
		} else {
			simplescim_scim_json_parser_syntax_error_expected(
				parser,
				"'case' or 'default'"
			);
			free(id);
			free(val);
			return -1;
		}

		free(id);
		simplescim_scim_json_parser_skip_ws(parser);
	}

	free(val);

	return 0;
}

/**
 * Parses a for statement.
 * <iter-start> ::= <ws>* <id>[1] <ws>* 'in' <ws>* <id>[2] <ws>* '}'
 *     [1] Must be iteration variable
 *     [2] Must be LDAP variable
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_scim_json_parser_iter_start(
	struct simplescim_scim_json_parser *parser
)
{
	char *iter_var, *ldap_var, *id;
	const struct simplescim_arbval_list *iter_val;
	struct simplescim_scim_json_iter *iter;
	int err;

	/* Skip <ws>* */
	simplescim_scim_json_parser_skip_ws(parser);

	/* Read iteration variable */
	if (parser->json[0] != '$') {
		simplescim_scim_json_parser_syntax_error_expected(
			parser,
			"iteration variable"
		);
		return -1;
	}

	iter_var = simplescim_scim_json_parser_id(parser);

	if (iter_var == NULL) {
		return -1;
	}

	/* Skip <ws>* */
	simplescim_scim_json_parser_skip_ws(parser);

	/* Read 'in' */
	id = simplescim_scim_json_parser_id(parser);

	if (id == NULL) {
		free(iter_var);
		return -1;
	}

	if (strcmp(id, "in") != 0) {
		simplescim_scim_json_parser_syntax_error_expected(
			parser,
			"'in'"
		);
		free(id);
		free(iter_var);
		return -1;
	}

	free(id);

	/* Skip <ws>* */
	simplescim_scim_json_parser_skip_ws(parser);

	/* Read LDAP variable */
	if (parser->json[0] == '$') {
		simplescim_scim_json_parser_syntax_error_expected(
			parser,
			"LDAP variable"
		);
		free(iter_var);
		return -1;
	}

	ldap_var = simplescim_scim_json_parser_id(parser);

	if (ldap_var == NULL) {
		free(iter_var);
		return -1;
	}

	/* Skip past <ws>* '}' */
	simplescim_scim_json_parser_skip_ws(parser);

	if (parser->json[0] != '}') {
		simplescim_scim_json_parser_syntax_error_expected(
			parser,
			"'}'"
		);
		free(ldap_var);
		free(iter_var);
		return -1;
	}

	simplescim_scim_json_parser_progress(parser);

	/* Get LDAP variable values */
	err = simplescim_user_get_attribute(
		parser->user,
		ldap_var,
		&iter_val
	);

	if (err == -1) {
		simplescim_scim_json_parser_syntax_error(parser);
		simplescim_error_string_set_message(
			"user does not have attribute \"%s\"",
			ldap_var
		);
		free(ldap_var);
		free(iter_var);
		return -1;
	}

	free(ldap_var);

	/* Allocate iteration stack node */
	iter = malloc(sizeof(struct simplescim_scim_json_iter));

	if (iter == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_scim_json_parser_iter_start:"
			"malloc"
		);
		free(iter_var);
		return -1;
	}

	iter->iter_var = strdup(iter_var + 1);

	if (iter->iter_var == NULL) {
		simplescim_error_string_set_errno(
			"simplescim_scim_json_parser_iter_start:"
			"strdup"
		);
		free(iter);
		free(iter_var);
		return -1;
	}

	free(iter_var);
	iter->iter_val = iter_val;
	iter->iter_idx = 0;
	iter->reset.json = parser->json;
	iter->reset.line = parser->line;
	iter->reset.col = parser->col;
	iter->next = parser->iteration_stack;
	parser->iteration_stack = iter;

	return 0;
}

/**
 * Parses an end statement.
 * <iter-end> ::= <ws>* '}'
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
static int simplescim_scim_json_parser_iter_end(
	struct simplescim_scim_json_parser *parser
)
{
	struct simplescim_scim_json_iter *iter;

	/* Skip past <ws>* '}' */
	simplescim_scim_json_parser_skip_ws(parser);

	if (parser->json[0] != '}') {
		simplescim_scim_json_parser_syntax_error_expected(
			parser,
			"'}'"
		);
		return -1;
	}

	simplescim_scim_json_parser_progress(parser);

	/* Get top of iteration stack */
	iter = parser->iteration_stack;

	if (iter == NULL) {
		simplescim_scim_json_parser_syntax_error(parser);
		simplescim_error_string_set_message(
			"end statement without matching for statement"
		);
		return -1;
	}

	/* FIXME: This case captures iterating over an
	   existing LDAP attribute with no values.
	   Currently, the iteration body will be printed
	   once with an empty string replacing the
	   iteration variable. Ideally, the iteration body
	   should not be printed at all. */
	if (iter->iter_idx == iter->iter_val->al_len) {
		parser->iteration_stack = iter->next;
		free(iter->iter_var);
		free(iter);
		return 0;
	}

	/* Increase the iteration value by one step */
	++iter->iter_idx;

	/* Check if it is the end of the iteration */
	if (iter->iter_idx == iter->iter_val->al_len) {
		parser->iteration_stack = iter->next;
		free(iter->iter_var);
		free(iter);
		return 0;
	}

	/* Go back to matching for statement for next iteration */
	parser->json = iter->reset.json;
	parser->line = iter->reset.line;
	parser->col = iter->reset.col;

	return 0;
}

/**
 * Parses a replacement rule.
 * <replace> ::= '${' <ws>* ( ( 'switch' <cond> | <id> ) <ws>* '}' )
 *                          | ( 'for' <iter-start> )
 *                          | ( 'end' <iter-end> )
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error string.
 */
static int simplescim_scim_json_parser_replacement(
	struct simplescim_scim_json_parser *parser
)
{
	char *id;
	int err;

	/* Skip past '${' <ws>* */
	simplescim_scim_json_parser_progress(parser);
	simplescim_scim_json_parser_progress(parser);
	simplescim_scim_json_parser_skip_ws(parser);

	/* Get id (variable name or keyword) */
	id = simplescim_scim_json_parser_id(parser);

	if (id == NULL) {
		return -1;
	}

	if (strcmp(id, "switch") == 0) {
		free(id);

		/* Conditional replacement */
		err = simplescim_scim_json_parser_replacement_cond(parser);

		if (err == -1) {
			return -1;
		}
	} else if (strcmp(id, "for") == 0) {
		free(id);

		/* Iteration replacement */
		err = simplescim_scim_json_parser_iter_start(parser);

		if (err == -1) {
			return -1;
		}

		return 0;
	} else if (strcmp(id, "end") == 0) {
		free(id);

		/* Iteration termination */
		err = simplescim_scim_json_parser_iter_end(parser);

		if (err == -1) {
			return -1;
		}

		return 0;
	} else {
		/* Simple replacement */
		err = simplescim_scim_json_parser_replacement_simple(
			parser,
			id
		);

		free(id);

		if (err == -1) {
			return -1;
		}
	}

	/* Skip past <ws>* '}' */
	simplescim_scim_json_parser_skip_ws(parser);

	if (parser->json[0] != '}') {
		simplescim_scim_json_parser_syntax_error_expected(
			parser,
			"'}'"
		);
		return -1;
	}

	simplescim_scim_json_parser_progress(parser);

	return 0;
}

/**
 * The main parser loop of the parser.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error string.
 */
static int simplescim_scim_json_parser_start(
	struct simplescim_scim_json_parser *parser
)
{
	int err;

	while (parser->json[0] != '\0') {
		/* Check if the current characters are '${', in which
		   case a replacement rule is applied. Otherwise, the
		   current character is copied to the output. */
		if (parser->json[0] == '$' && parser->json[1] == '{') {
			err = simplescim_scim_json_parser_replacement(
				parser
			);

			if (err == -1) {
				return -1;
			}
		} else {
			err = simplescim_scim_json_parser_copy_char(
				parser,
				parser->json[0]
			);

			if (err == -1) {
				return -1;
			}

			simplescim_scim_json_parser_progress(parser);
		}
	}

	/* Check iteration balancing */

	if (parser->iteration_stack != NULL) {
		simplescim_scim_json_parser_syntax_error(parser);
		simplescim_error_string_set_message(
			"unmatched iteration statement"
		);
		return -1;
	}

	return 0;
}

/**
 * Parses the input JSON template string 'json' and
 * replaces specified values with values from 'user'.
 * On success, the parsed output JSON string is returned.
 * On error, NULL is returned and simplescim_error_string
 * is set to an appropriate error message.
 */
char *simplescim_scim_json_parse(
	const char *json,
	const struct simplescim_user *user
)
{
	struct simplescim_scim_json_parser *parser;
	char *output;
	int err;

	/* Allocate parser */
	parser = simplescim_scim_json_parser_new(json, user);

	if (parser == NULL) {
		return NULL;
	}

	/* Start parsing */
	err = simplescim_scim_json_parser_start(parser);

	if (err == -1) {
		simplescim_scim_json_parser_delete(parser);
		return NULL;
	}

	/* Terminate output string */
	parser->output.str[parser->output.len] = '\0';

	/* Get output string and clean up */
	output = parser->output.str;
	parser->output.str = NULL;
	simplescim_scim_json_parser_delete(parser);

	return output;
}
