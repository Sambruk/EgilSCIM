#include <utility>

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

#include "scim_json_parse.hpp"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <optional>
#include <algorithm>

#include "utility/simplescim_error_string.hpp"
//#include "model/value_list.hpp"
#include "model/base_object.hpp"
#include "utility/utils.hpp"

typedef std::optional<std::string> optional_string;
typedef std::string::const_iterator str_iter;

typedef std::map<std::string, string_vector> value_map;

struct scim_json_iter {
	value_map iter_value;
	size_t iter_idx = 0;
	size_t max_iter = 0;
	struct {
		str_iter resetJson;
		size_t line = 0;
		size_t col = 0;
	} reset;

	scim_json_iter() = default;

	scim_json_iter(const std::string &first, const string_vector &second) {
		max_iter = second.size();
		iter_value.emplace(std::make_pair(first, second));
	}

	scim_json_iter(value_map &&map) {
		for (const auto &item : map) {
			if (item.second.size() > max_iter)
				max_iter = item.second.size();
		}
		iter_value = std::move(map);
	}

	bool more() {
		return iter_idx < max_iter;
	}

	void set_next(std::shared_ptr<scim_json_iter> n) {
		next = std::move(n);
	}

	std::shared_ptr<scim_json_iter> get_next() {
		return next;
	}

private:
	std::shared_ptr<scim_json_iter> next;
};

class scim_json_parser {
public:
	str_iter j_iter;

	const base_object &user;
	size_t line;
	size_t col;
	std::string output_string;

	std::shared_ptr<scim_json_iter> iteration_stack;

	scim_json_parser(const std::string &j, const base_object &u) :
			j_iter(j.begin()), user(u), line(1), col(1), iteration_stack(nullptr) {}

	/**
	 * Deletes a simplescim_scim_json_parser object.
	 */
	~scim_json_parser() {
		std::shared_ptr<scim_json_iter> iter;

		while (iteration_stack != nullptr) {
			iter = iteration_stack;
			iteration_stack = iter->get_next();
		}
	}

	void remove_trailing_commas();

	/**
	 * Progresses the parser's input by one character.
	 */
	void progress();

	/**
	 * Copies the character 'c' to the output JSON string.
	 * On success, zero is returned. On error, -1 is returned
	 * and simplescim_error_string is set to an appropriate
	 * error message.
	 */
	int copy_char(char c);

	/**
	 * Progresses the parser's input past any optional white
	 * space characters.
	 */
	void skip_ws();

	/**
	 * If 'c' is a character in an ID, 1 is returned.
	 * Otherwise, 0 is returned.
	 */
	int is_id(char c);

	/**
	 * Parses an ID in the input JSON string.
	 * <id> ::= '$'? [a-zA-Z0-9\-\_]+
	 * On success, the ID is returned. On error,
	 * nullptr is returned and simplescim_error_string is set to
	 * an appropriate error message.
	 */
	std::string find_id();

	/**
	 * Writes 'val' to the output JSON string.
	 * On success, zero is returned. On error, -1 is returned
	 * and simplescim_error_string is set to an appropriate
	 * error string.
	 */
	int replace(const std::string &value);

	/**
	 * Gets the first value of the LDAP attribute or iteration
	 * variable 'var'.
	 * On success, a pointer to the value is returned. On
	 * error, nullptr is returned and simplescim_error_string is
	 * set to an appropriate error message.
	 */
	optional_string get_value(const std::string &variable);

	/**
	 * Parses a simple replacement rule.
	 * On success, zero is returned. On error, -1 is returned
	 * and simplescim_error_string is set to an appropriate
	 * error string.
	 */
	int replacement_simple(const std::string &var) {
		int err;

		/* Get value */
		optional_string val = get_value(var);

		if (!val) {
			return -1;
		}

		/* Write replacement */
		err = replace(*val);

		if (err == -1) {
			return -1;
		}

		return 0;
	}

	/**
	 * Parses a string literal, either single or double quoted.
	 * <string> ::= '\'' [^']* '\''
	 *            | '"' [^"]* '"'
	 * On success, a pointer to the parsed string is returned.
	 * On error, nullptr is returned and simplescim_error_string
	 * is set to an appropriate error message.
	 */
	std::string parse_string();

	/**
	 * Parses a case statement.
	 * <case> ::= <ws>* <string> <ws>* ':' <ws>* <string>
	 * On success, zero is returned. On error, -1 is returned
	 * and simplescim_error_string is set to an appropriate
	 * error message.
	 */
	int parse_case(const std::string &inval, int *matched);

	/**
	 * Parses a default statement.
	 * <default> ::= <ws>* ':' <ws>* <string>
	 * On success, zero is returned. On error, -1 is returned
	 * and simplescim_error_string is set to an appropriate
	 * error message.
	 */
	int parse_default(int matched);

	/**
	 * Parses a conditional replacement rule.
	 * <cond> ::= <ws>* <id> <ws>* ('case' <case> <ws>*)*
	 *                             'default' <default> <ws>*
	 * On success, zero is returned. On error, -1 is returned
	 * and simplescim_error_string is set to an appropriate
	 * error string.
	 */
	int replacement_conditional();

	/**
	 * Parses a for statement.
	 * <iter-start> ::= <ws>* <id>[1] <ws>* 'in' <ws>* <id>[2] <ws>* '}'
	 *     [1] Must be iteration variable
	 *     [2] Must be LDAP variable
	 * On success, zero is returned. On error, -1 is returned
	 * and simplescim_error_string is set to an appropriate
	 * error message.
	 */
	int iter_start();

	/**
	 * Parses an end statement.
	 * <iter-end> ::= <ws>* '}'
	 * On success, zero is returned. On error, -1 is returned
	 * and simplescim_error_string is set to an appropriate
	 * error message.
	 */
	int iter_end();

	/**
	 * Parses a replacement rule.
	 * <replace> ::= '${' <ws>* ( ( 'switch' <cond> | <id> ) <ws>* '}' )
	 *                          | ( 'for' <iter-start> )
	 *                          | ( 'end' <iter-end> )
	 * On success, zero is returned. On error, -1 is returned
	 * and simplescim_error_string is set to an appropriate
	 * error string.
	 */
	int parse_replacement();

	/**
	 * The main parser loop of the parser.
	 * On success, zero is returned. On error, -1 is returned
	 * and simplescim_error_string is set to an appropriate
	 * error string.
	 */
	int parse();

	/**
	 * Sets the prefix of simplescim_error_string to a syntax
	 * error.
	 */
	void syntax_error();

	/**
	 * Sets simplescim_error_string to a syntax error in the
	 * format "expected x, found y".
	 */
	void syntax_error_expected(const char *expected);

};

void scim_json_parser::syntax_error() {
	simplescim_error_string_set_prefix("scim_json_parser:%lu:%lu:syntax error", line, col);
}

void scim_json_parser::syntax_error_expected(const char *expected) {
	syntax_error();

	if (isprint(*j_iter)) {
		simplescim_error_string_set_message("expected %s, found '%c'", expected, *j_iter);
	} else {
		simplescim_error_string_set_message("expected %s, found 0x%02X", expected, *j_iter);
	}
}

int scim_json_parser::parse() {
	int err;

	while (*j_iter != '\0') {
		/* Check if the current characters are '${', in which
		   case a replacement rule is applied. Otherwise, the
		   current character is copied to the output. */
		if (*j_iter == '$' && *(j_iter + 1) == '{') {
			err = parse_replacement();

			if (err == -1) {
				return -1;
			}
		} else {
			err = copy_char(*j_iter);
			if (err == -1) {
				return -1;
			}

			progress();
		}
	}

	/* Check iteration balancing */

	if (iteration_stack != nullptr) {
		syntax_error();
		simplescim_error_string_set_message("unmatched iteration statement");
		return -1;
	}

	remove_trailing_commas();
	return 0;
}

/**
 * Arrays has a comma at the end, e.g.
 * ["tag":"value","tag":"value","tag":"value",]
 * that last one needs to go, it's wrong and
 * boost::propertytree doesn't accept it
 */
void scim_json_parser::remove_trailing_commas() {

	auto end = output_string.end();
	bool found_block_end;
	bool inside_string = false;

	for (auto && iter = output_string.begin(); iter != end; iter++) {

		// let strings have commas so
		// pop in and out of strings
		if (*iter == '\"')
			inside_string = !inside_string;

		if (!inside_string && *iter == ',') {
			auto walker = iter;
			// the next character can be whitespace of \"
			// if it is ] or }, the comma needs to be erased
			found_block_end = false;
			while (walker != end && !found_block_end) {
				if (*walker == '\"')
					break; // all good, move on
				if (*walker == ']' || *walker == '}') {
					*iter = ' ';
					found_block_end = true;
				}
				walker++;
			}
		}
	}
}


int scim_json_parser::parse_replacement() {
	int err;

	/* Skip past '${' <ws>* */
	progress();
	progress();
	skip_ws();

	/* Get id (variable name or keyword) */
	std::string id = find_id();

	if (id.empty()) {
		return -1;
	}

	if (id == "switch") {

		/* Conditional replacement */
		err = replacement_conditional();

		if (err == -1) {
			return -1;
		}
	} else if (id == "for") {

		/* Iteration replacement */
		err = iter_start();

		if (err == -1) {
			return -1;
		}

		return 0;
	} else if (id == "end") {
		/* Iteration termination */
		err = iter_end();

		if (err == -1) {
			return -1;
		}

		return 0;
	} else {
		/* Simple replacement */
		err = replacement_simple(id);

		if (err == -1) {
			return -1;
		}
	}

	/* Skip past <ws>* '}' */
	skip_ws();

	if (*j_iter != '}') {
		syntax_error_expected("'}'");
		return -1;
	}

	progress();

	return 0;
}

int scim_json_parser::iter_end() {

	/* Skip past <ws>* '}' */
	skip_ws();

	if (*j_iter != '}') {
		syntax_error_expected("'}'");
		return -1;
	}

	progress();

	/* Get top of iteration stack */
	std::shared_ptr<scim_json_iter> iter = iteration_stack;

	if (iter == nullptr) {
		syntax_error();
		simplescim_error_string_set_message("end statement without matching for statement");
		return -1;
	}

	/* FIXME: This case captures iterating over an
	   existing LDAP attribute with no values.
	   Currently, the iteration body will be printed
	   once with an empty string replacing the
	   iteration variable. Ideally, the iteration body
	   should not be printed at all. */
	if (!iter->more()) {
		iteration_stack = iter->get_next();
		return 0;
	}

	/* Increase the iteration value by one step */
	++iter->iter_idx;

	/* Check if it is the end of the iteration */
	if (!iter->more()) {
		iteration_stack = iter->get_next();
		return 0;
	}

	/* Go back to matching for statement for next iteration */
	j_iter = iter->reset.resetJson;
	line = iter->reset.line;
	col = iter->reset.col;

	return 0;
}

int scim_json_parser::iter_start() {


	/* Skip <ws>* */
	skip_ws();

	/* Read iteration variable */
	if (*j_iter != '$') {
		syntax_error_expected("iteration variable");
		return -1;
	}

	string_vector iter_variables;
	for (std::string iter_var; iter_var != "in";) {
		iter_var = find_id();

		if (iter_var.empty()) {
			return -1;
		}
		if (iter_var == "in")
			continue;
		else
			iter_variables.push_back({iter_var.begin() + 1, iter_var.end()});

		/* Skip <ws>* */
		skip_ws();

		if (iter_variables.size() > 100) {
			syntax_error_expected("'in'");
			return -1;
		}
	}
	/* Skip <ws>* */
	skip_ws();

	/* Read LDAP variable */
	if (*j_iter == '$') {
		syntax_error_expected("LDAP variable");
		return -1;
	}
	string_vector ldap_variables;
	for (std::string ldap_var = find_id(); !ldap_var.empty(); ldap_var = find_id()) {

		if (!ldap_var.empty())
			ldap_variables.emplace_back(ldap_var);
		if (*j_iter == '}')
			break;
		skip_ws();
	}


	if (ldap_variables.empty() || ldap_variables.size() != iter_variables.size()) {
		std::cerr << line << " " << col << " number of iteration variables and attributes must match" << std::endl;
		return -1;
	}

	/* Skip past <ws>* '}' */
	skip_ws();

	if (*j_iter != '}') {
		syntax_error_expected("'}'");
		return -1;
	}

	progress();

	/* Get LDAP variable values */

	value_map map;
	for (unsigned int i = 0; i < ldap_variables.size(); i++) {
		const string_vector &values = user.get_values(ldap_variables.at(i));
		map.emplace(std::make_pair(iter_variables.at(i), values));
	}
	auto iter = std::make_shared<scim_json_iter>(std::move(map));

	iter->iter_idx = 0;
	iter->reset.resetJson = j_iter;
	iter->reset.line = line;
	iter->reset.col = col;
	iter->set_next(iteration_stack);
	iteration_stack = iter;

	return 0;
}

int scim_json_parser::replacement_conditional() {
	int matched = 0;

	skip_ws();

	/* Get variable */
	std::string id = find_id();

	/* Get value */
	optional_string val = get_value(id);

	if (!val) {
		return -1;
	}

	skip_ws();

	/* Iterate over all case and default statements */
	for (;;) {
		id = find_id();

		if (id.empty()) {
			return -1;
		}

		if (id == "case") {
			parse_case(*val, &matched);
		} else if (id == "default") {
			parse_default(matched);
			break;
		} else {
			syntax_error_expected("'case' or 'default'");
			return -1;
		}

		skip_ws();
	}

	return 0;
}

int scim_json_parser::parse_default(int matched) {
	int err;

	skip_ws();

	if (*j_iter != ':') {
		syntax_error_expected("':'");
		return -1;
	}

	progress();
	skip_ws();
	std::string value = parse_string();

	if (value.empty()) {
		return -1;
	}

	if (matched == 0) {
		err = replace(value);

		if (err == -1) {
			return -1;
		}
	}

	return 0;
}

int scim_json_parser::parse_case(const std::string &inval, int *matched) {
	std::string match, value;
	int err;

	skip_ws();
	match = parse_string();

	if (match.empty()) {
		return -1;
	}

	skip_ws();

	if (*j_iter != ':') {
		syntax_error_expected("':'");
		return -1;
	}

	progress();
	skip_ws();
	value = parse_string();

	if (*matched == 0 && match == inval) {
		err = replace(value);

		if (err == -1) {
			return -1;
		}

		*matched = 1;
	}

	return 0;
}

std::string scim_json_parser::parse_string() {
	char quote;
	size_t len;
	size_t i;

	/* Determine quote type */
	if (*j_iter == '\'') {
		quote = '\'';
	} else if (*j_iter == '"') {
		quote = '"';
	} else {
		syntax_error_expected("single or double quote");
		return "";
	}

	progress();

	/* Determine string length */
	len = 0;

	if (quote == '\'') {
		while (*(j_iter + len) != '\'' && *(j_iter + len) != '\0') {
			++len;
		}
	} else {
		while (*(j_iter + len) != '\"' && *(j_iter + len) != '\0') {
			++len;
		}
	}

	if (*(j_iter + len) == '\0') {
		syntax_error();
		simplescim_error_string_set_message("unexpected end-of-string");
		return "";
	}

	std::string str(j_iter, j_iter + len);

	/* Progress parser */
	for (i = 0; i <= len; ++i) {
		progress();
	}

	return str;
}

optional_string scim_json_parser::get_value(const std::string &variable) {
	std::string return_value;
	str_iter var = std::begin(variable);
	str_iter iterEnd = std::end(variable);
	if (*var == '$') {
		++var;

		/* Get iteration variable */
		std::shared_ptr<scim_json_iter> iter = iteration_stack;

		while (iter != nullptr) {
			std::string var_string(var, iterEnd);
			const value_map &val = iter->iter_value;
			auto value = val.find(var_string);
			if (var_string == value->first) {
				if (iter->iter_idx >= value->second.size()) {
					return_value = "";
				} else {
					return_value = value->second.at(iter->iter_idx);
				}

				break;
			}

			iter = iter->get_next();
		}

		if (iter == nullptr) {
			syntax_error();
			simplescim_error_string_set_message(R"(iteration variable "%s" does not exist)", var);
			return {};
		}
	} else {
		const string_vector &values = user.get_values(std::string(var, iterEnd));

		if (values.empty()) {
			syntax_error();

			simplescim_error_string_set_message(R"("%s" "%s" does not have attribute "%s" )",
			                                    user.getSS12000type().c_str(), user.get_uid(false).c_str(), var);
			return {};
		}

		if (values.empty()) {
			return_value = "";
		} else {
			return_value = values.at(0);
		}
	}

	return return_value;
}

int scim_json_parser::replace(const std::string &value) {
	output_string += value;
	return 0;
}

std::string scim_json_parser::find_id() {
	size_t len, i;

	/* Check for iteration variable */
	if (*j_iter == '$') {
		len = 1;
	} else {
		len = 0;
	}

	/* At least one ID character */
	if (!is_id(*(j_iter + len))) {
		syntax_error_expected("variable name or keyword");
		return "";
	}

	/* Determine id length */
	while (is_id(*(j_iter + len))) {
		++len;
	}

	std::string id(j_iter, j_iter + len);

	/* Progress the parser */
	for (i = 0; i < len; ++i) {
		progress();
	}

	return id;
}

int scim_json_parser::is_id(char c) {
	if (isalnum(c) || c == '-' || c == '_' || c == '.') {
		return 1;
	} else {
		return 0;
	}
}

void scim_json_parser::skip_ws() {
	while (isspace(*j_iter)) {
		progress();
	}
}

int scim_json_parser::copy_char(char c) {

	output_string += c;
	return 0;
}

void scim_json_parser::progress() {
	if (*j_iter == '\n') {
		++line;
		col = 1;
	} else {
		++col;
	}

	++j_iter;
}

/**
 * Parses the input JSON template string 'json' and
 * replaces specified values with values from 'user'.
 * On success, the parsed output JSON string is returned.
 * On error, nullptr is returned and simplescim_error_string
 * is set to an appropriate error message.
 */
std::string scim_json_parse(const std::string &json, const base_object &user) {
	int err;

	scim_json_parser parser(json, user);
	err = parser.parse();


	if (err == -1) {
		return "";
	}

	return parser.output_string;
}
