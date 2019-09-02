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

#ifndef EGILSCIM_JSON_TEMPLATE_PARSER_HPP
#define EGILSCIM_JSON_TEMPLATE_PARSER_HPP

#include <string>
#include <set>

namespace JSONTemplateParser {

typedef std::string::const_iterator iter;

/*
 * Searches through a string to find any variables (${foo}).
 *
 * Used to find variables in the JSON templates.
 */
std::set<std::string> find_variables(iter start, iter end);

}

#endif // EGILSCIM_JSON_TEMPLATE_PARSER_HPP
