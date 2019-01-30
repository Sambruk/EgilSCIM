/**
 * Copyright © 2017-2018  Max Wällstedt <>
 *
 * This file is part of EgilSCIM.
 *
 * EgilSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EgilSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with EgilSCIM.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Further development with groups and relations support
 * by Ola Mattsson - IT informa for Sambruk
 */

#include "simplescim_ldap_attrs_parser.hpp"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "utility/simplescim_error_string.hpp"
#include "config_file.hpp"

/**
 * A global data structure containing the current
 * state and position of the parser.
 */
class Parser {
public:
	const char *cur = nullptr;
	size_t line = 0;
	size_t col = 0;
	char **attrs = nullptr;
	size_t n_attrs = 0;

	void reset() {
		cur = nullptr;
		line = 0;
		col = 0;
		attrs = nullptr;
		n_attrs = 0;
	}

	/**
	 * Prints a syntax error to simplescim_error_string
	 * according to global 'parser' object and 'str'.
	 */
	void syntax_error(const std::string &str) {
		simplescim_error_string_set_prefix(
				"%s:ldap-attrs:%lu:%lu:syntax error", config_file::instance().file_name(),
				line, col);
		simplescim_error_string_set_message("%s", str.c_str()
		);
	}

	/**
	 * Prints a syntax error to 'simplescim_error_string'
	 * according to global static 'parser' object and 'str',
	 * when the error is of type "expected x, found y".
	 */
	void syntax_error_expected(const std::string &str) {
		simplescim_error_string_set_prefix(
				"%s:ldap-attrs:%lu:%lu:syntax error", config_file::instance().file_name(),
				line, col);

		if (isprint(*cur)) {
			simplescim_error_string_set_message(
					"expected %s, found '%c'", str.c_str(), *cur);
		} else {
			simplescim_error_string_set_message(
					"expected %s, found 0x%02X", str.c_str(), *cur);
		}
	}

	void skip_ws() {
		while (*cur == ' '
		       || *cur == '\t'
		       || *cur == '\n') {
			if (*cur == '\n') {
				++cur;
				++line;
				col = 1;
			} else {
				++cur;
				++col;
			}
		}
	}

	int parse_attr(char **dest) {
		size_t attr_len = 0;
		char *attr;

		skip_ws();

		while (cur[attr_len] != ' '
		       && cur[attr_len] != '\t'
		       && cur[attr_len] != '\n'
		       && cur[attr_len] != ','
		       && cur[attr_len] != '\0') {
			++attr_len;
		}

		if (attr_len == 0) {
			syntax_error("empty attribute name");
			return -1;
		}

		attr = static_cast<char *>(malloc(attr_len + 1));

		if (attr == nullptr) {
			simplescim_error_string_set_errno(
					"parse_attr:"
					"malloc"
			);
			return -1;
		}

		memcpy(attr, cur, attr_len);
		attr[attr_len] = '\0';
		*dest = attr;

		cur += attr_len;
		col += attr_len;

		skip_ws();

		return 0;
	}
};

/**
 * Parses a comma separated list of attributes into a
 * nullptr-terminated list of strings.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int simplescim_ldap_attrs_parser(const char *attrs, char ***attrs_val) {
	size_t i;
	int err;
	Parser parser;
	parser.cur = attrs;
	parser.line = 1;
	parser.col = 1;
	parser.n_attrs = 1;

	/* If no attributes are specified, set *dest to nullptr */
	if (strcmp(attrs, "") == 0) {
		*attrs_val = nullptr;
		return 0;
	}

	/* Find number of attributes */
	for (i = 0; attrs[i] != '\0'; ++i) {
		if (attrs[i] == ',') {
			++parser.n_attrs;
		}
	}

	/* Allocate attrs */
	parser.attrs = static_cast<char **>(malloc(sizeof(char *) * (parser.n_attrs + 1)));

	if (parser.attrs == nullptr) {
		simplescim_error_string_set_errno(
				"simplescim_ldap_attrs_parser:" "malloc");
		parser.reset();
		return -1;
	}

	/* Parse all attributes */
	for (i = 0; i < parser.n_attrs; ++i) {
		if (i > 0) {
			if (*parser.cur != ',') {
				parser.syntax_error_expected("','");

				while (i > 0) {
					free(parser.attrs[i - 1]);
					--i;
				}

				free(parser.attrs);
				parser.reset();

				return -1;
			}

			++parser.cur;
			++parser.col;
		}

		err = parser.parse_attr(&parser.attrs[i]);

		if (err == -1) {
			while (i > 0) {
				free(parser.attrs[i - 1]);
				--i;
			}

			free(parser.attrs);
			parser.reset();

			return -1;
		}
	}

	if (*parser.cur != '\0') {
		parser.syntax_error_expected("end-of-string");

		while (i > 0) {
			free(parser.attrs[i - 1]);
			--i;
		}

		free(parser.attrs);
		parser.reset();

		return -1;
	}

	parser.attrs[parser.n_attrs] = nullptr;
	*attrs_val = parser.attrs;
	parser.reset();

	return 0;
}
