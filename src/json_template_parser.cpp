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
#include <regex>
#include <iostream>

namespace JSONTemplateParser {

using namespace std;

bool handle_switch(const string& str, string& variable) {
    smatch m;
    if (regex_match(str, m, regex("\\$\\{[[:space:]]*switch[[:space:]]*((\\w|\\.)+)[[:space:]](.|\n)*\\}"))) {
        variable = m[1].str();
        return true;
    }
    else {
        return false;
    }
}

bool handle_for_header(const string& str, set<string>& variables) {
    smatch m;
    if (regex_match(str, m, regex("\\$\\{[[:space:]]*for[[:space:]](.|\\n)*?[[:space:]]in[[:space:]](([[:space:]]*(\\w|\\.)+[[:space:]]*)*)\\}"))) {
        if (m.size() > 2) {
            string variable_sequence = m[2].str();
            istringstream is(variable_sequence);
            string var;
            while (is >> var) {
                variables.insert(var);
            }
        }
        return true;
    }
    else {
        return false;
    }
}

bool handle_variable(const string& str, string& variable) {
    smatch m;
    if (regex_match(str, m, regex("\\$\\{[[:space:]]*((\\w|\\.)+)[[:space:]]*\\}"))) {
        if (m.size() > 1 && m[1].str() != "end") {
            variable = m[1].str();
            return true;
        }
    }
    return false;
}

set<string> find_variables(iter start, iter end) {
    string var;
    set<string> vars;
    smatch m;
    bool found = regex_search(start, end, m, regex("\\$\\{(.|\\n)*?\\}"));
    if (!found) {
        return {};
    }
    
    auto result = find_variables(m.suffix().first, m.suffix().second);
    
    if (handle_switch(m[0].str(), var)) {
        result.insert(var);
    }
    else if (handle_for_header(m[0].str(), vars)) {
        result.insert(vars.begin(), vars.end());
    }
    else if (handle_variable(m[0].str(), var)) {
        result.insert(var);
    }
    else {
        // ${end}, unknown ${...}-sequence or a variable which shouldn't be
        // included (for instance iteration variables like ${$i}), just ignore it
    }
    return result;
}

}
