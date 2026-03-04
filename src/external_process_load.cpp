/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2026 Föreningen Sambruk
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

#include "external_process_load.hpp"
#include "config_file.hpp"
#include "config.hpp"
#include "transformer.hpp"
#include "load_limiter.hpp"
#include "load_common.hpp"
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace pt = boost::property_tree;

namespace {

struct type_ep_settings {
    std::string session;
    std::string args;
};

type_ep_settings parse_type_settings(const std::string& json) {
    type_ep_settings result;

    try {
        std::stringstream json_stream;
        json_stream << json;
        pt::ptree root;
        pt::read_json(json_stream, root);
        result.session = root.get<std::string>("session");
        result.args = root.get<std::string>("args", "");
    } catch (const pt::ptree_error& e) {
        throw std::runtime_error(std::string("Failed to parse external-process settings: ") + e.what());
    }

    return result;
}

} // anonymous namespace

std::shared_ptr<object_list> json_to_object_list(const std::string& json,
                                                  const std::string& type) {
    auto objects = std::make_shared<object_list>();
    const auto ignore_dups = config::ignore_duplicate_uuids();

    try {
        std::stringstream json_stream;
        json_stream << json;
        pt::ptree root;
        pt::read_json(json_stream, root);

        for (auto& element : root) {
            auto& obj_node = element.second;

            attrib_map attributes;
            attributes["ss12000type"] = string_vector({type});

            for (auto& prop : obj_node) {
                auto& key = prop.first;
                auto& value_node = prop.second;

                if (value_node.empty() && !value_node.data().empty()) {
                    // Simple string value
                    attributes[key] = string_vector({value_node.data()});
                } else if (!value_node.empty() && value_node.data().empty()
                           && value_node.front().first.empty()) {
                    // Array of strings (array items have empty keys in ptree)
                    string_vector values;
                    for (auto& item : value_node) {
                        values.push_back(item.second.data());
                    }
                    attributes[key] = std::move(values);
                }
                // Skip nested objects (children with named keys) and nulls
            }

            auto object = std::make_shared<base_object>(std::move(attributes));

            if (config_file::instance().has(type + "-UUID-generator")) {
                generate_uuid(object,
                              config_file::instance().get(type + "-UUID-generator"),
                              config_file::instance().get(type + "-unique-identifier"));
            }

            auto uid = object->get_uid();

            if (!ignore_dups && objects->has_object(uid)) {
                throw std::runtime_error("External process output for type " + type +
                                         " contained duplicate UUID: " + uid);
            }

            if (!uid.empty()) {
                auto acceptable_uuid = warn_if_bad_uuid(uid);
                if (acceptable_uuid) {
                    objects->add_object(uid, object);
                }
            }
        }
    } catch (const pt::ptree_error& e) {
        throw std::runtime_error(std::string("Failed to parse JSON from external process: ") + e.what());
    }

    return objects;
}

json_array_splitter::json_array_splitter(process_sink& inner)
    : inner_(inner) {}

void json_array_splitter::write(const char* data, size_t len) {
    if (failed_ || done_) return;

    for (size_t i = 0; i < len; ++i) {
        char c = data[i];

        if (!seen_array_start_) {
            if (std::isspace(static_cast<unsigned char>(c))) continue;
            if (c == '[') {
                seen_array_start_ = true;
                continue;
            }
            failed_ = true;
            return;
        }

        if (brace_depth_ == 0) {
            // Between objects: expect whitespace, comma, '{', or ']'
            if (std::isspace(static_cast<unsigned char>(c)) || c == ',') continue;
            if (c == ']') {
                done_ = true;
                return;
            }
            if (c == '{') {
                brace_depth_ = 1;
                current_object_ = "{";
                continue;
            }
            failed_ = true;
            return;
        }

        // Inside an object
        current_object_ += c;

        if (in_string_) {
            if (escape_next_) {
                escape_next_ = false;
            } else if (c == '\\') {
                escape_next_ = true;
            } else if (c == '"') {
                in_string_ = false;
            }
            continue;
        }

        if (c == '"') {
            in_string_ = true;
        } else if (c == '{') {
            ++brace_depth_;
        } else if (c == '}') {
            --brace_depth_;
            if (brace_depth_ == 0) {
                inner_.write(current_object_.data(), current_object_.size());
                current_object_.clear();
            }
        }
    }
}

std::shared_ptr<object_list> external_process_get(const external_process_manager& manager,
                                                   const std::string& type,
                                                   indented_logger& load_logger) {
    load_logger.log(std::string("Loading entries for type ") + type + " from external process");
    indented_logger::indenter indenter(load_logger);

    auto settings_json = config_file::instance().get(type + "-external-process");
    auto settings = parse_type_settings(settings_json);

    string_sink stdout_data;
    stderr_sink err_sink;
    auto exit_code = manager.run_command(settings.session, settings.args, stdout_data, err_sink);

    if (exit_code != 0) {
        throw std::runtime_error("External process for type " + type + " (session \"" +
                                 settings.session + "\") failed with exit code " +
                                 std::to_string(exit_code));
    }

    auto objects = json_to_object_list(stdout_data.content, type);

    auto transform = get_transformer(type);
    transform_objects(objects, transform);

    auto limiter = get_limiter(type);
    objects = filter_objects(objects, limiter, load_logger, type);

    load_related(type, objects, load_logger);

    return objects;
}
