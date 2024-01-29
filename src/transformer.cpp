/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2022 Föreningen Sambruk
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

#include "transformer.hpp"

#include "transformer_impl.hpp"
#include "config_file.hpp"
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

std::vector<regex_transform_rule> parse_regex_transforms(const pt::ptree& root) {
    std::vector<regex_transform_rule> result;
    try {
        for (auto child : root) {
            std::vector<std::string> params;
            for (auto grandchild : child.second) {
                params.push_back(grandchild.second.get_value<std::string>());
            }
            if (params.size() != 3) {
                throw std::runtime_error("incorrect number of elements in transforms array");
            }
            auto regex = params[0];
            auto to = params[1];
            auto replace = params[2];

            result.push_back(regex_transform_rule(regex, to, replace));
        }
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(std::string("failed to parse transforms: ") + e.what());
    }
    return result;
}

std::shared_ptr<transformer> create_attribute_transformer_from_json(const pt::ptree& root) {
    auto from = root.get<std::string>("from");
    auto function = root.get("function", "regex");

    if (function == "regex") {
        try {
            auto transforms = parse_regex_transforms(root.get_child("transforms"));
            auto matchAll = root.get("matchAll", true);
            auto noMatch = root.get("noMatch", "");

            return std::make_shared<regex_transformer>(from, transforms, matchAll, noMatch);            
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(std::string("failed to create regex transformer: ") + e.what());
        }
    }
    else if (function == "urldecode") {
        try {
            auto to = root.get<std::string>("to", from); // if to isn't specified we'll replace the from attribute
            return std::make_shared<urldecode_transformer>(from, to);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error(std::string("failed to create urldecode transformer: ") + e.what());
        }
    }
    else {
        throw std::runtime_error("failed to parse attribute transformer, unknown function \"" + function + "\"");
    }

    return std::make_shared<null_transformer>();
}

std::shared_ptr<transformer> create_multi_attribute_transformer_from_json(const pt::ptree& root) {
    auto result = std::make_shared<multi_attribute_transformer>();

    for (auto child : root) {
        auto attribute_transformer = create_attribute_transformer_from_json(child.second);
        result->add(attribute_transformer);
    }

    return result;
}

std::shared_ptr<transformer> create_transformer(const std::string& type) {
    config_file &conf = config_file::instance();

    if (conf.has(type + "-transform-attributes")) {
        std::stringstream json_stream;
        json_stream << conf.get(type + "-transform-attributes");
        
        pt::ptree root;
        try {
            pt::read_json(json_stream, root);
        } catch (const std::runtime_error &e) {
            throw std::runtime_error(std::string("failed to parse transformer: ") + " (" + e.what() + ")");
        }
        return create_multi_attribute_transformer_from_json(root);
    }
    else {
        return std::make_shared<null_transformer>();
    }
}

std::shared_ptr<transformer> get_transformer(const std::string& type) {
    static std::map<std::string, std::shared_ptr<transformer>> transformers;

    if (transformers.find(type) == transformers.end()) {
        try {
            transformers[type] = create_transformer(type);
        } catch (const std::runtime_error& e) {
            throw std::runtime_error("Failed to create transformer for "+ type + " (" + e.what() + ")");
        } 
    }
    return transformers[type];
}

std::vector<std::string> get_transformed_attributes(const std::string& type) {
    std::vector<std::string> result;
    config_file &conf = config_file::instance();

    if (conf.has(type + "-transform-attributes")) {
        std::stringstream json_stream;
        json_stream << conf.get(type + "-transform-attributes");
        
        pt::ptree root;
        try {
            pt::read_json(json_stream, root);
        } catch (const std::runtime_error &e) {
            throw std::runtime_error("Failed to parse transformer: " + type + " (" + e.what() + ")");
        }
        for (auto child : root) {
            result.push_back(child.second.get<std::string>("from"));
        }
    }
    return result;
}