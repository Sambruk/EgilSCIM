/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2024 Föreningen Sambruk
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

#include "load_limiter.hpp"
#include "config_file.hpp"
#include "load_limiter_impl.hpp"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

namespace {
    std::shared_ptr<load_limiter> user_blacklist;
}

void set_user_blacklist(const std::string &filename,
                        const std::string &attribute) {
    user_blacklist = std::make_shared<not_limiter>(std::make_shared<list_limiter>(filename, attribute));
}

/** If there is a user blacklist set, this function will return a new limiter based on the
 *  passed in limiter. The returned limiter will work like the passed in limiter except that
 *  it will exclude users from the blacklist.
 * 
 *  If no user blacklist is set the limiter will be returned as it is.
 * 
*/
std::shared_ptr<load_limiter> combine_with_user_blacklist(std::shared_ptr<load_limiter> limiter) {
    if (user_blacklist == nullptr) {
        return limiter;
    }
    else {
        return std::make_shared<and_limiter>(std::vector<std::shared_ptr<load_limiter>>{limiter, user_blacklist});
    }
}

std::shared_ptr<load_limiter> create_limiter_from_json(const pt::ptree& root);

std::vector<std::shared_ptr<load_limiter>> create_limiters_from_json_array(const pt::ptree& root) {
    std::vector<std::shared_ptr<load_limiter>> result;
    for (const auto& child : root) {
        result.push_back(create_limiter_from_json(child.second));
    }
    return result;
}

std::shared_ptr<load_limiter> create_limiter_from_json(const pt::ptree& root) {
    auto limit_type = toUpper(root.get<std::string>("with"));

    if (limit_type == "LIST") {
        auto filename = config_file::instance().interpret_config_path(root.get<std::string>("list"));
        auto attribute = root.get<std::string>("by", "");
        return std::make_shared<list_limiter>(filename, attribute);
    }
    else if (limit_type == "REGEX") {
        auto regex = root.get<std::string>("regex");
        auto attribute = root.get<std::string>("by");
        return std::make_shared<regex_limiter>(regex, attribute);
    }
    else if (limit_type == "NOT") {
        return std::make_shared<not_limiter>(create_limiter_from_json(root.get_child("child")));
    }
    else if (limit_type == "AND") {
        return std::make_shared<and_limiter>(create_limiters_from_json_array(root.get_child("children")));
    }
    else if (limit_type == "OR") {
        return std::make_shared<or_limiter>(create_limiters_from_json_array(root.get_child("children")));
    }
    else {
        throw std::runtime_error("No such limit type: " + limit_type);
    }
}

/** Returns the limiter for a specific type, or for the SCIM endpoint
 *  if there's no type specific limiter (or a null_limiter if there's
 *  no endpoint limiter).
 *  Note that if there's a user blacklist, it is not created here, the
 *  caller of this function is expected to combine the limiter with
 *  the user blacklist if the type is a user type.
 */
std::shared_ptr<load_limiter> create_limiter(const std::string& type) {
    config_file &conf = config_file::instance();

    if (conf.has(type + "-limit")) {
        std::stringstream json_stream;
        json_stream << conf.get(type + "-limit");
        
        pt::ptree root;
        try {
            pt::read_json(json_stream, root);
        } catch (const boost::exception &ex) {
            throw std::runtime_error("Failed to parse limiter: " + type);
        }
        return create_limiter_from_json(root);
    }
    else if (conf.has(type + "-limit-with")) {
        auto limit_type = toUpper(conf.get(type + "-limit-with"));

        if (limit_type == "LIST") {
            auto filename = conf.get_path(type + "-limit-list");
            auto attribute = conf.get(type + "-limit-by", true);
            return std::make_shared<list_limiter>(filename, attribute);
        }
        else if (limit_type == "REGEX") {
            auto regex = conf.get(type + "-limit-regex");
            auto attribute = conf.get(type + "-limit-by");
            return std::make_shared<regex_limiter>(regex, attribute);
        }
        else {
            throw std::runtime_error("No such limit type: " + limit_type);
        }
    }
    else {
        // If there isn't a limiter for this specific type, see if there's one
        // for the endpoint (for instance if there isn't a limiter for Teacher, try
        // Users). This way we can specify a limiter for all users.
        auto endpoint_variable = type + "-scim-url-endpoint";
        if (conf.has(endpoint_variable) && conf.get(endpoint_variable) != type) {
            return create_limiter(conf.get(endpoint_variable));
        } 
        return std::make_shared<null_limiter>();
    }
}

std::shared_ptr<load_limiter> get_limiter(const std::string& type) {
    config_file &conf = config_file::instance();

    static std::string current_config;
    static std::map<std::string, std::shared_ptr<load_limiter>> limiters;

    if (conf.file_name_str() != current_config) {
        limiters.clear();
        current_config = conf.file_name_str();
    }

    if (limiters.find(type) == limiters.end()) {
        auto l = create_limiter(type);

        // Figure out if we should add the user blacklist to the limiter
        auto endpoint_variable = type + "-scim-url-endpoint";
        if (conf.has(endpoint_variable) && conf.get(endpoint_variable) == "Users") {
            limiters[type] = combine_with_user_blacklist(l);
        } else {
            limiters[type] = l;
        }
    }
    return limiters[type];
}
