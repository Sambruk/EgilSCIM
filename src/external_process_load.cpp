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
#include <boost/json/src.hpp>

namespace pt = boost::property_tree;

namespace {

std::string value_to_string(const boost::json::value& v) {
    if (v.is_string()) return std::string(v.get_string());
    if (v.is_int64()) return std::to_string(v.get_int64());
    if (v.is_uint64()) return std::to_string(v.get_uint64());
    if (v.is_double()) return std::to_string(v.get_double());
    if (v.is_bool()) return v.get_bool() ? "true" : "false";
    return {};
}

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

json_parser_sink::json_parser_sink(std::shared_ptr<object_list> objects, const std::string& type)
    : objects_(std::move(objects)), type_(type), ignore_dups_(config::ignore_duplicate_uuids()) {}

void json_parser_sink::write(const char* data, size_t len) {
    if (failed_) return;

    try {
        auto obj_val = boost::json::parse(boost::json::string_view(data, len));
        if (!obj_val.is_object()) {
            failed_ = true;
            error_message_ = "Expected JSON object from external process";
            return;
        }
        auto& obj = obj_val.get_object();

        attrib_map attributes;
        attributes["ss12000type"] = string_vector({type_});

        for (auto& kv : obj) {
            auto key = std::string(kv.key());
            auto& val = kv.value();

            if (val.is_null()) {
                continue;
            } else if (val.is_array()) {
                string_vector values;
                for (auto& item : val.get_array()) {
                    if (!item.is_null() && !item.is_object() && !item.is_array()) {
                        values.push_back(value_to_string(item));
                    }
                }
                attributes[key] = std::move(values);
            } else if (val.is_object()) {
                continue;
            } else {
                attributes[key] = string_vector({value_to_string(val)});
            }
        }

        auto object = std::make_shared<base_object>(std::move(attributes));

        if (config_file::instance().has(type_ + "-UUID-generator")) {
            generate_uuid(object,
                          config_file::instance().get(type_ + "-UUID-generator"),
                          config_file::instance().get(type_ + "-unique-identifier"));
        }

        auto uid = object->get_uid();

        if (!ignore_dups_ && objects_->has_object(uid)) {
            failed_ = true;
            error_message_ = "External process output for type " + type_ +
                             " contained duplicate UUID: " + uid;
            return;
        }

        if (!uid.empty()) {
            auto acceptable_uuid = warn_if_bad_uuid(uid);
            if (acceptable_uuid) {
                objects_->add_object(uid, object);
            }
        }
    } catch (const std::exception& e) {
        failed_ = true;
        error_message_ = std::string("Failed to parse JSON from external process: ") + e.what();
    }
}

json_array_splitter::json_array_splitter(process_sink& inner)
    : inner_(inner) {}

void json_array_splitter::write(const char* data, size_t len) {
    if (failed_ || done_) return;

    for (size_t i = 0; i < len; ++i) {
        char c = data[i];

        if (!seen_first_char_) {
            if (std::isspace(static_cast<unsigned char>(c))) continue;
            seen_first_char_ = true;
            if (c == '[') {
                is_array_ = true;
                continue;
            }
            if (c == '{') {
                brace_depth_ = 1;
                current_object_ = "{";
                continue;
            }
            failed_ = true;
            return;
        }

        if (brace_depth_ == 0) {
            if (std::isspace(static_cast<unsigned char>(c))) continue;
            if (is_array_ && c == ',') continue;
            if (is_array_ && c == ']') {
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

    auto objects = std::make_shared<object_list>();
    json_parser_sink parser(objects, type);
    json_array_splitter splitter(parser);
    stderr_sink err_sink;
    auto exit_code = manager.run_command(settings.session, settings.args, splitter, err_sink);

    if (exit_code != 0) {
        throw std::runtime_error("External process for type " + type + " (session \"" +
                                 settings.session + "\") failed with exit code " +
                                 std::to_string(exit_code));
    }

    if (splitter.failed()) {
        throw std::runtime_error("Failed to parse JSON from external process for type " + type);
    }

    if (splitter.incomplete()) {
        throw std::runtime_error("External process for type " + type +
                                 " produced truncated JSON output");
    }

    if (parser.failed()) {
        throw std::runtime_error(parser.error_message());
    }

    auto transform = get_transformer(type);
    transform_objects(objects, transform);

    auto limiter = get_limiter(type);
    objects = filter_objects(objects, limiter, load_logger, type);

    load_related(type, objects, load_logger);

    return objects;
}
