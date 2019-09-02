/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2019 Föreningen Sambruk
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.

 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef SIMPLESCIM_CONFIG_FILE_PARSER_H
#define SIMPLESCIM_CONFIG_FILE_PARSER_H

#include <string>

/**
 * Parses the configuration file with its contents in
 * 'input'.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
class config_parser {
	using iter = std::string::const_iterator;
	iter start;
	iter cur;
	iter end;
	size_t line;
	size_t col;
public:
	explicit config_parser(iter first, iter last) : start(first), cur(first), end(last),
	                                                line(0), col(0) {};

	~config_parser() = default;

	/**
	 * <config> ::= ( <ws>* <assign>? <comment>? '\n' )*
	 * */
	int parse();

	void reset() {
		cur = start;
		line = 0;
		col = 0;
	}

private:
	/**
	 * Returns 1 if 'c' is a valid character in a varid.
	 * Returns 0 otherwise.
	 */
	int is_varid(char c);

	/**
	 * <ws> ::= ' ' | '\t'
	 */
	void rule_skip_ws();

	/**
	 * <varid> ::= [-_a-zA-Z0-9]+
	 */
	int rule_varid(std::string &varp);

	/**
	 * <value> ::= '<?' [^('?>')]* '?>' <ws>*
	 *           | [^('#'|'\n')]*                    # remove trailing <ws>*
	 */
	int rule_value(std::string &valp);

	/**
	 * <assign> ::= <varid> <ws>* '=' <ws>* <value>
	 * */
	int rule_assign();

	void advance(size_t dist = 1);

	int advance_to(char c);

	void next_line();

	/* <comment> ::= '#' [^\n]* */
	int rule_comment();

	void syntax_error(const std::string &str);

	void syntax_error_expected(const std::string &str);

};

#endif
