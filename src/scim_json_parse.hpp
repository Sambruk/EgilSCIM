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

#ifndef SIMPLESCIM_SCIM_JSON_H
#define SIMPLESCIM_SCIM_JSON_H

#include <string>

class base_object;

/**
 * Parses the input JSON template string 'json' and
 * replaces specified values with values from 'object'.
 * On success, the parsed output JSON string is returned.
 * On error, "" is returned and simplescim_error_string
 * is set to an appropriate error message.
 */
std::string scim_json_parse(const std::string &json, const base_object &object);

#endif
