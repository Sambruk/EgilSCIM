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

#include "generated_group_load.hpp"
#include "config_file.hpp"
#include "data_server.hpp"
#include <regex>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

/**
 * Parses the specification of one of the attributes from which we
 * generate groups.
 */
student_group_attribute parse_student_group_attribute(const pt::ptree& spec) {
    student_group_attribute result;
    result.from = spec.get<std::string>("from");
    result.match = spec.get<std::string>("match");
    result.uuid = spec.get<std::string>("uuid");

    auto attrs = spec.get_child("attributes");
    for (auto child : attrs) {
        std::pair<std::string, std::string> attribute;
        auto itr = child.second.begin();
        if (itr == child.second.end()) {
            throw std::runtime_error("empty attribute specification for student group attribute");
        }
        attribute.first = (*itr).second.get_value<std::string>();
        ++itr;
        if (itr == child.second.end()) {
            throw std::runtime_error("expected second value for attribute specification for student group attribute");
        }
        attribute.second = (*itr).second.get_value<std::string>();
        result.attributes.push_back(attribute);
    }
    return result;
}

/**
 * Parses the list of attributes from which we generate groups.
 * 
 * Throws runtime_error on malformed specification.
 */
std::vector<student_group_attribute> parse_student_group_attributes(const std::string& spec) {
    std::vector<student_group_attribute> result;

    try {
        pt::ptree root;
        std::stringstream json_stream;
        json_stream << spec;
        pt::read_json(json_stream, root);

        for (auto child : root) {
            auto attribute = parse_student_group_attribute(child.second);
            result.push_back(attribute);
        }
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Failed to parse generate-from-attributes for virtual student groups: " + std::string(e.what()));
    }
    return result;
}

/**
 * Generates virtual groups generated from attributes on other objects
 * (typically Students and Teachers).
 */
std::shared_ptr<object_list> get_generated_student_group(const std::string& type,
                                                         indented_logger& load_logger) {
    config_file& conf = config_file::instance();
    data_server &server = data_server::instance();
    auto generated = std::make_shared<object_list>();

    auto from_types = conf.get_vector_sorted_unique(type + "-generate-from-types");

    auto attributes = parse_student_group_attributes(conf.get(type + "-generate-from-attributes"));

    string_vector scim_vars = conf.get_vector_sorted_unique(type + "-scim-variables");

    for (auto from_type : from_types) {
        auto user_list = server.get_by_type(from_type);

        if (!user_list) {
            continue;
        }

        for (const auto& user: *user_list) {

            for (const auto& attribute : attributes) {
                // Get the "from" attribute from the user
                auto from_values = user.second->get_values(attribute.from);

                for (const auto& from : from_values) {
                    // Match against the regular expression
                    if (!std::regex_match(from, attribute.match)) {
                        continue;
                    }

                    // Generate the string which will be used to generate the UUID for the group
                    auto uuid_basis = std::regex_replace(from, attribute.match, attribute.uuid, std::regex_constants::format_no_copy);
                    auto uuid = uuid_util::instance().generate(uuid_basis);

                    // Do we need to create the group?
                    bool created_now = false;
                    auto group = generated->get_object(uuid);
                    if (!group) {
                        created_now = true;
                        group = std::make_shared<base_object>(type);
                        group->add_attribute(conf.get(type + "-unique-identifier"), {uuid});
                        // Create the group's attributes from the "from" attribute
                        for (const auto& attr : attribute.attributes) {
                            auto name = attr.first;
                            auto value = std::regex_replace(from, attribute.match, attr.second, std::regex_constants::format_no_copy);
                            group->add_attribute(name, {value});
                        }
                    }

                    // Establish relation between group and user
                    for (auto &&var : scim_vars) {
                        auto p = string_to_pair(var);
                        if (p.first == from_type) {
                            string_vector v = user.second->get_values(p.second);
                            group->append_values(var, v);
                        }
                    }

                    auto relations_scim_vars = conf.get_vector(from_type + "-scim-variables");
                    for (auto &&var : relations_scim_vars) {
                        auto p = string_to_pair(var);
                        if (p.first == type) {
                            auto id = group->get_values(p.second);
                            user.second->append_values(type + "." + p.second, id, true);
                        }
                    }

                    // If new, add the group to the results
                    if (created_now) {
                        generated->add_object(uuid, group);
                        load_logger.log(std::string("Generated ") + type + " with UUID " + uuid +
                            " from " + from_type + " with UUID " + user.second->get_uid());
                    }
                }
            }

        }
    }

    return generated;
}

/**
 * The attributes from which we generate the groups are typically
 * not mentioned in the templates, so to make sure they're read from
 * LDAP we'll need to add them to *-scim-variables here.
 */
void add_scim_vars_for_virtual_groups() {
    config_file &config = config_file::instance();
    // Only do this if StudentGroups are actually generated
    if (!config.get_bool("StudentGroup-is-generated")) {
        return;
    }

    auto attributes = parse_student_group_attributes(config.get("StudentGroup-generate-from-attributes"));
    auto types = config.get_vector_sorted_unique("StudentGroup-generate-from-types");

    for (auto type : types) {
        for (const auto& attribute : attributes) {
            auto var = attribute.from;
            config.add_variable(type + "-scim-variables", var);
            config.add_variable("all-scim-variables", var);
        }
    }
}
