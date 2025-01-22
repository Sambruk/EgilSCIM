/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2025 Föreningen Sambruk
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

#include "print_cache.hpp"
#include "config_file.hpp"
#include <algorithm>
#include <iostream>

#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

namespace {
/** Checks if all where conditions matches the json.
  * A where condition is something like externalId=foo
  * Paths can also be used to get sub-attributes, such as name/givenName=Babs
  * boost::property_tree paths are used with / as separator.
  */
bool where_matches(const std::vector<std::string> &where, const std::string& json) {
    // No need to parse JSON if there are no where conditions
    if (where.size() == 0) {
        return true;
    }
    pt::ptree root;
    try {
        std::stringstream json_stream;
        json_stream << json;
        pt::read_json(json_stream, root);
    }
    catch (const std::runtime_error &) {
        // Unparsable JSON isn't printed if we have where conditions
        return false;
    }

    bool all_matched = true;
    for (auto &condition : where) {
        std::string variable, value;
        try {
            parse_override(condition, variable, value);
            if (root.get<std::string>(pt::ptree::path_type{variable, '/'}) != value) {
                all_matched = false;
                break;
            }
        } catch (std::runtime_error& e) {
            // either we couldn't parse the condition, or the object
            // didn't have the variable
            all_matched = false;
            break;
        }
    }
    return all_matched;
}
}

void print_cache(std::shared_ptr<rendered_object_list> cache,
                 bool by_endpoint,
                 const std::vector<std::string> &types,
                 const std::vector<std::string> &where) {

    config_file &conf = config_file::instance();
    std::map<std::string, std::vector<std::string>> to_print_per_type;

    for (auto &itr : *cache) {
        auto obj = itr.second;

        auto obj_type = obj->get_type();

        if (by_endpoint) {
            auto endpoint_variable = obj_type + "-scim-url-endpoint";
            if (conf.has(endpoint_variable)) {
                obj_type = conf.get(endpoint_variable);
            }
        }

        if (types.size() == 0 || std::find(types.begin(), types.end(), obj_type) != types.end()) {
            auto json = obj->get_json();
            if (where_matches(where, json)) {
                to_print_per_type[obj_type].push_back(json);
            }
        }
    }

    std::cout << "{\n";

    bool first_type = true;
    for (auto &itr : to_print_per_type) {
        if (!first_type) {
            std::cout << ",\n";
        }
        auto& current_type = itr.first;
        auto& current_objects = itr.second;

        std::cout << "\"" << current_type << "\": [\n";

        bool first_object = true;
        for (auto &jtr : current_objects) {
            if (!first_object) {
                std::cout << ",\n";
            }
            std::cout << jtr << "\n";
            first_object = false;
        }

        std::cout << "]";
        first_type = false;
    }

    std::cout << "\n}" << std::endl;
}