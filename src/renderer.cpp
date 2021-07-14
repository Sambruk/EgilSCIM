/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2021 Föreningen Sambruk
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

#include "renderer.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "config_file.hpp"
#include "scim_json_parse.hpp"
#include "utility/simplescim_error_string.hpp"

namespace {

/*
 * This function converts from the type names used in the EGIL
 * client configuration to SS12000 types.
 *
 * Typically the type names are the same, except for Student and Teacher
 * which both map to User in SS12000.
 *
 * If the type has a config variable named `type`-SS12000-type, for instance:
 * 
 * Student-SS12000-type = User
 *
 * then that will be used. Otherwise type will be returned unchanged for 
 * everything except Student and Teacher.
 */
std::string actualSS12000type(const std::string& type) {
    auto config_var = type + "-SS12000-type";

    if (config_file::instance().has(config_var)) {
        return config_file::instance().get(config_var);
    }
    else {
        if (type == "Teacher" ||
            type == "Student") {
            return "User";
        }
        else {
            return type;
        }
    }
}

}

std::shared_ptr<rendered_object> renderer::render(const post_processing::plugins& ppp, const base_object& obj) {
    std::string type = obj.getSS12000type();
    std::string standard_type = actualSS12000type(type);

    std::string template_json = config_file::instance().get(type + "-scim-json-template");
    std::string parsed_json = scim_json_parse(template_json, obj);
    
    if (parsed_json == "") {
        throw std::runtime_error("failed to parse JSON template for " + type);
    }

    if (!verify_json(parsed_json, type)) {
        throw std::runtime_error("failed to parse JSON template for " + type);
    }

    try {
        parsed_json = post_processing::process(ppp, standard_type, parsed_json);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("post processing error when creating object " + obj.get_uid() + ": " + e.what());
    }
    return std::make_shared<rendered_object>(obj.get_uid(), type, parsed_json);
}

bool renderer::verify_json(const std::string & json, const std::string &type) {
    if (json.empty()) {
        return false;
    }
    else if (std::find(verified_types.begin(), verified_types.end(), type) != verified_types.end()) {
        return true;
    }

    namespace pt = boost::property_tree;
    pt::ptree root;
    std::stringstream os;

    os << json;
    try {
        pt::read_json(os, root);
        verified_types.emplace_back(type);
    } catch (const pt::ptree_error& e) {
        std::cerr << "Failed to parse JSON for " << type << std::endl;
        simplescim_error_string_set_message(e.what());
        return false;
    }
    return true;
}
