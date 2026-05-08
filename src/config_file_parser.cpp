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
#include "utility/utils.hpp"


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

void config_parser::parse() {
	/** Zero or more lines */
	while (cur != end) {
		/** Optional white space */
		rule_skip_ws();

		if (cur == end) {
			break;
        }

		/** Optional variable assignment */
		if (is_varid(*cur)) {
			rule_assign();
		}

		if (cur == end) {
			break;
        }

		/** Optional comment */
		if (*cur == '#') {
			rule_comment();
		}

		if (cur == end) {
			break;
        }

		/** Obligatory newline */
		if (*cur != '\n') {
			syntax_error_expected("end-of-line");
		}

		next_line();
	}
}

void config_parser::next_line() {
	++cur;
	++line;
	col = 1;
}

void config_parser::rule_skip_ws() {
	while (cur != end && (*cur == ' ' || *cur == '\t')) {
		advance();
    }
}

void config_parser::rule_varid(std::string &varp) {
	std::string var;
	size_t var_len = 0;

	/* Determine variable name length */
	while (cur + var_len < end && is_varid(*(cur + var_len))) {
		++var_len;
	}

	if (var_len == 0) {
		syntax_error("empty variable name");
	}

	varp.assign(cur, cur + var_len);

	advance(var_len);
}

void config_parser::rule_value(std::string &valp) {
	size_t val_len = 0;

	/** Multi line value or single line value */
	if (*cur == '<' && ((cur + 1) != end && *(cur + 1) == '?')) {
		size_t tmp_line, tmp_col;

		advance(2);

		tmp_line = line;
		tmp_col = col;

		/** Determine length of multi line value */
		for (;;) {
			if (cur + val_len == end) {
				line = tmp_line;
				col = tmp_col;
				syntax_error("unexpected end-of-file");
			}

			/** Multi line value terminated by '?>' */
			if (*(cur + val_len) == '?') {
				if (cur + val_len + 1 == end) {
					line = tmp_line;
					col = tmp_col;
					syntax_error("unexpected end-of-file");
                }
				else if (*(cur + val_len + 1) == '>') {
					break;
				}
                // else just regular ? in multi line value, continue searching for '?>'
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
		while ((cur + val_len) != end 
				&& *(cur + val_len) != '\n'
				&& *(cur + val_len) != '#') {
			++val_len;
		}

		valp.assign(cur, cur + val_len);

		advance(val_len);

		/** Remove trailing white space */
		valp.erase(valp.find_last_not_of(" \n\r\t") + 1);

	}
}

void config_parser::rule_assign() {
	/** Obligatory variable name */
	if (!is_varid(*cur)) {
		syntax_error_expected("variable name");
	}

	std::string var;
	rule_varid(var);

	/** Optional white space */
	rule_skip_ws();

	if (cur == end) {
		syntax_error("unexpected end-of-file");
    }

	/** Obligatory variable assignment character */
	if (*cur != '=') {
		syntax_error_expected("'='");
	}

	advance();

	/* Optional white space */
	rule_skip_ws();

	/* Obligatory value */
	std::string val;

    // Don't attempt to get the value if we are at the end of the file, but do allow an empty value even at the end of the file. 
	// This allows for a file that ends with "key=" without a newline at the end.
	if (cur != end) {
		rule_value(val);
	}

	inserter_(var, val);
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

void config_parser::rule_skip_rest_of_line() {
	while (cur != end && *cur != '\n') {
		advance();
	}
}

void config_parser::rule_comment() {
	/** Obligatory line comment initialiser character */
	if (*cur != '#') {
		syntax_error_expected("'#'");
	}

	rule_skip_rest_of_line();
}

void config_parser::syntax_error(const std::string &str) {
    throw config_parse_error(line, col, "syntax error: " + str);
}

void config_parser::syntax_error_expected(const std::string &str) {
	if (isprint(static_cast<unsigned char>(*cur))) {
        auto msg = string_format("syntax error: expected %s, found '%c'", str.c_str(), *cur);
        throw config_parse_error(line, col, msg);
	} else {
		auto msg = string_format("syntax error: expected %s, found 0x%02X", str.c_str(), static_cast<unsigned char>(*cur));
        throw config_parse_error(line, col, msg);
	}
}

