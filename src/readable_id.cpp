/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2023 Föreningen Sambruk
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


#include "readable_id.hpp"
#include "config_file.hpp"
#include <map>

using namespace std;

string readable_id(const base_object* obj, const string t) {
    auto type = t;
    if (t == "") {
        type = obj->getSS12000type();
    }

    static map<string, string> attributes_to_use;

    auto itr = attributes_to_use.find(type);

    if (itr == attributes_to_use.end()) {
        attributes_to_use[type] = config_file::instance().get(type + "-readable-id", true);
    }

    auto attribute = attributes_to_use[type];
    string value = "";

    if (attribute != "") {
        auto values = obj->get_values(attribute);
        if (!values.empty()) {
            value = values.front();
        }
        else {
            value = "<unset>";
        }
    }

    string result = string("UUID=") + obj->get_uid();

    if (attribute != "" && value != "") {
        result = attribute + "=" + value + " (" + result + ")";
    }

    return result;
}
