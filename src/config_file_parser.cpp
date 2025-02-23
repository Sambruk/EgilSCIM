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

#include "config_file_parser.hpp"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <iostream>
#include <algorithm>
#include <set>
#include <vector>
#include "utility/simplescim_error_string.hpp"
#include "config_file.hpp"


int config_parser::is_varid(char c) {
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

	return (int) lookup_table[(unsigned char) c];
}

int config_parser::parse() {
	int err;

	/** Zero or more lines */
	while (cur != end) {
		/** Optional white space */
		rule_skip_ws();

		/** Optional variable assignment */
		if (is_varid(*cur)) {
			err = rule_assign();

			if (err == -1) {
				return -1;
			}
		}

		/** Optional comment */
		if (*cur == '#') {
			err = rule_comment();

			if (err == -1) {
				return -1;
			}
		}

		/** Obligatory newline */
		if (*cur != '\n') {
			syntax_error_expected("end-of-line, there should be a new line after ?>");
			return -1;
		}

		next_line();
	}

	return 0;
}

void config_parser::next_line() {
	++cur;
	++line;
	col = 1;
}

void config_parser::rule_skip_ws() {
	advance(std::string(cur, end).find_first_not_of(" \t"));
}

int config_parser::rule_varid(std::string &varp) {
	std::string var;
	size_t var_len = 0;

	/* Determine variable name length */
	while (is_varid(*(cur + var_len))) {
		++var_len;
	}

	if (var_len == 0) {
		syntax_error("empty variable name");
		return -1;
	}

	varp.assign(cur, cur + var_len);

	advance(var_len);

	return 0;
}

int config_parser::rule_value(std::string &valp) {
	size_t val_len = 0;

	/** Multi line value or single line value */
	if (*cur == '<' && *(cur + 1) == '?') {
		size_t tmp_line, tmp_col;

		advance(2);

		tmp_line = line;
		tmp_col = col;

		/** Determine length of multi line value */
		for (;;) {
			if (*(cur + val_len) == '\0') {
				line = tmp_line;
				col = tmp_col;
				syntax_error("unexpected end-of-file");
				return -1;
			}

			/** Multi line value terminated by '?>' */
			if (*(cur + val_len) == '?' && *(cur + val_len + 1) == '>') {
				break;
			}

			if (*(cur + val_len) == '\n') {
				++val_len;
				++tmp_line;
				tmp_col = 1;
			} else {
				++val_len;
				++tmp_col;
			}
		}

		valp.assign(cur, cur + val_len);
		cur += val_len + 2;
		col = tmp_col + 2;
		line = tmp_line;

		rule_skip_ws();
	} else {
		/** Determine single line value length */
		while (*(cur + val_len) != '\n'
		       && *(cur + val_len) != '#'
		       && (cur + val_len) != end) {
			++val_len;
		}

		if (cur + val_len == end) {
			col += val_len;
			syntax_error("unexpected end-of-file");
			return -1;
		}

		valp.assign(cur, cur + val_len);

		advance(val_len);

		/** Remove trailing white space */
		valp.erase(valp.find_last_not_of(" \n\r\t") + 1);

	}

	return 0;
}

int config_parser::rule_assign() {
	int err;

	/** Obligatory variable name */
	if (!is_varid(*cur)) {
		syntax_error_expected("variable name");
		return -1;
	}

	std::string var;
	err = rule_varid(var);

	if (err == -1) {
		return -1;
	}

	/** Optional white space */
	rule_skip_ws();

	/** Obligatory variable assignment character */
	if (*cur != '=') {
		syntax_error_expected("'='");
		return -1;
	}

	advance();

	/* Optional white space */
	rule_skip_ws();

	/* Obligatory value */
	std::string val;
	err = rule_value(val);

	if (err == -1) {
		return -1;
	}

	err = config_file::instance().insert(var, val);

	if (err == -1) {
		return -1;
	}

	return 0;
}

int config_parser::advance_to(const char c) {
	size_t pos = std::string(cur, end).find(c);
	if (pos == std::string::npos)
		return -1;
	advance(pos);
	return 0;
}

void config_parser::advance(size_t dist) {
	cur += dist;
	col += dist;
}

int config_parser::rule_comment() {
	/** Obligatory line comment initialiser character */
	if (*cur != '#') {
		syntax_error_expected("'#'");
		return -1;
	}

	advance();

	if (advance_to('\n') == -1 || cur == end) {
		syntax_error("unexpected end-of-file");
		return -1;
	}

	return 0;
}

void config_parser::syntax_error(const std::string &str) {
	/** Set prefix */
	auto config = config_file::instance().file_name_str();
	simplescim_error_string_set_prefix("%s:%lu:%lu:syntax error", config.c_str(),
	                                   line, col);

	/** Set message */
	simplescim_error_string_set_message("%s", str.c_str());
}

void config_parser::syntax_error_expected(const std::string &str) {
	/** Set prefix */
	auto config = config_file::instance().file_name_str();
	simplescim_error_string_set_prefix("%s:%lu:%lu:syntax error", config.c_str(),
	                                   line, col);

	/** Set message */
	if (isprint(*cur)) {
		simplescim_error_string_set_message(
				"expected %s, found '%c'",
				str.c_str(),
				*cur
		);
	} else {
		simplescim_error_string_set_message(
				"expected %s, found 0x%02X",
				str.c_str(),
				*cur
		);
	}
}

