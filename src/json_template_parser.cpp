/**
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
 */

#include "json_template_parser.hpp"
#include <algorithm>
#include <vector>
#include <iostream>
#include "utility/utils.hpp"

namespace JSONTemplateParser {

void skip_ws(iter& cur, iter end) {
    while (cur != end && (*cur == ' ' || *cur == '\t')) {
        ++cur;
    }
}

std::set<std::string> find_variables(const std::string &type, iter start, iter end) {

    auto cur = start;
    
    std::set<std::string> var_set;

    while (cur != end) {
        iter var_start = std::find(cur, end, '$');
        iter array_start = std::find(cur, end, '[');

        if (array_start < var_start) {
            iter array_end = std::find(array_start, end, ']');
            if (array_end != end) {
                size_t for_pos = std::string(array_start, array_end).find("${for");
                if (for_pos == std::string::npos) {
                    cur = array_start + 2;
                    continue;
                }
                cur = array_start + for_pos;
                if (cur < array_end) {
                    cur = cur + std::string(cur, array_end).find("in");
                    if (cur < array_end) {
                        cur += 2;
                        skip_ws(cur, end);

                        std::vector<std::string> tmp = string_to_vector({cur, std::find(cur, array_end, '}')});
                        var_set.insert(tmp.begin(), tmp.end());
                    }
                }
                cur = array_end + 1;
                continue;
            }
        }
        cur = var_start;
        if (cur != end) {
            if (*(cur + 1) != '{') {
                // a dollar sign without curly brace is just a dollar sign
                cur++;
                continue;
            }
            /** see if it's a switch */
            size_t switch_pos = std::string(cur, end).find("switch");
            if (switch_pos != std::string::npos) {
                iter block_end = std::find(cur, end, '}');
                if (cur + switch_pos < block_end) {
                    cur += std::string("{switch ").length();
                    skip_ws(cur, end);
                    iter var_end = cur + std::string(cur, block_end).find_first_of(" \n\t}");
                    var_set.insert({cur, var_end});
                    continue;
                }

            }

            cur += 2;
            skip_ws(cur, end);
            auto v_end = std::find(cur, end, '}');
            if (v_end != end) {
                std::string variable(cur, v_end);
                if (variable.find_first_of(" \n\t") != std::string::npos) {
                    std::cerr << R"(syntax error "${" needs corresponding "}")" << std::endl;
                }

                var_set.insert(variable);
                cur = v_end;
            } else {
                std::cerr << R"(syntax error "${" needs corresponding "}")" << std::endl;
                return std::set<std::string>();
            }
        }
    }

    return var_set;
}

}
