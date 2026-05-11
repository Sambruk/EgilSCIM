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

#include <stdexcept>
#include <string>
#include <functional>

/**
 * Exception thrown by config_parser when a syntax error is encountered.
 * Callers are responsible for prepending the filename to form the full
 * error location string.
 * line() and col() return the line and column of the error.
 * what() returns the error message.
 */
class config_parse_error : public std::runtime_error {
public:
	explicit config_parse_error(size_t line, size_t col, const std::string &msg) : std::runtime_error(msg), line_(line), col_(col) {}
    size_t line() const { return line_; }
    size_t col() const { return col_; }
private:
	size_t line_;
	size_t col_;
};

/**
 * Parses the configuration file with its contents in 'input'.
 * Throws config_parse_error on syntax errors.
 */
class config_parser {
	using iter = std::string::const_iterator;
	iter start;
	iter cur;
	iter end;
	size_t line;
	size_t col;

public:
	using insert_fn = std::function<void(const std::string&, const std::string&)>;

	/**
	 * Constructor for the config_parser.
	 * @param first Iterator to the beginning of the input string.
	 * @param last Iterator to the end of the input string.
	 * @param inserter Function to insert parsed key-value pairs.
	 */
	explicit config_parser(iter first, iter last,
							insert_fn inserter) : start(first), cur(first), end(last),
	                                                line(0), col(0), inserter_(inserter) {};

	~config_parser() = default;

	/**
	 * <config> ::= ( <ws>* <assign>? <comment>? '\n' )*
	 * Throws config_parse_error on failure.
	 * */
	void parse();

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
	void rule_varid(std::string &varp);

	/**
	 * <value> ::= '<?' [^('?>')]* '?>' <ws>*
	 *           | [^('#'|'\n')]*                    # remove trailing <ws>*
	 */
	void rule_value(std::string &valp);

	/**
	 * <assign> ::= <varid> <ws>* '=' <ws>* <value>
	 * */
	void rule_assign();

	void advance(size_t dist = 1);

	int advance_to(char c);

	void rule_skip_rest_of_line();

	void next_line();

	/* <comment> ::= '#' [^\n]* */
	void rule_comment();

	void syntax_error(const std::string &str);

	void syntax_error_expected(const std::string &str, char found);
	
	insert_fn inserter_;
};

#endif
