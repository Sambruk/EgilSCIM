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

#include "config_file.hpp"

#include <map>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <vector>
#include <set>

#include "utility/simplescim_error_string.hpp"
#include "config_file_parser.hpp"
#include "json_template_parser.hpp"
#include "json_data_file.hpp"
#include "transformer.hpp"
#include "utility/utils.hpp"

int config_file::load_templates() {
    int err = 0;
    for (auto &&variable : variables) {
        std::string::size_type type_end = variable.first.find("-scim-conf");
        if (type_end != std::string::npos) {
            std::string::const_iterator iter = std::begin(variable.first);
            std::string type = {iter, iter + type_end};
            load_template(type, variable.second);
        }
    }
    return err;
}

using namespace std::experimental;

int config_file::load_template(const std::string &ss12000type, const std::string &file) {
    int err = 0;
    std::string content = read(filesystem::canonical(file, filename.parent_path()));

    if (content.empty()) {
        std::cerr << ss12000type << "-scim-conf requested but the file is missing" << std::endl;
        return -1;
    }
    config_parser parser(std::begin(content), std::end(content));
    err = parser.parse();
    if (!err) {
        auto json_template = get(ss12000type + "-scim-json-template");
        auto var_set = JSONTemplateParser::find_variables(json_template.begin(),
                                                          json_template.end());
        
        auto extra = get_vector(ss12000type + "-hidden-attributes", true);
        if (!extra.empty())
            var_set.insert(extra.begin(), extra.end());

        relations_vector relations =
            json_data_file::json_to_ldap_remote_relations(
                get(ss12000type + "-remote-relations", true), ss12000type);

        for (auto& relation : relations) {
            if (!relation.local_attribute.empty()) {
                var_set.insert(relation.local_attribute);
            }
            if (!relation.remote_attribute.empty()) {
                add_variable(relation.type + "-scim-variables", relation.remote_attribute);
                add_variable("all-scim-variables", relation.remote_attribute);
            }
        }

        auto transformed_attributes = get_transformed_attributes(ss12000type);
        for (auto attr : transformed_attributes) {
            var_set.insert(attr);   
        }

        std::string variables;
        for (const auto &var : var_set) {
            if (var.empty()) {
                continue;
            }
            
            auto typePos = var.find('.');
            if (typePos != std::string::npos) {
                std::string foreignKey(var.substr(typePos + 1));
                std::string typeForKey(var.substr(0, typePos));
                add_variable(typeForKey + "-scim-variables", foreignKey);
            }
            variables += var + ", ";
        }
        const std::string attribute = ss12000type + "-scim-variables";
        if (!variables.empty()) {
            variables.erase(variables.end() - 2, variables.end());
            add_variable(attribute, variables);
            add_variable("all-scim-variables", variables);
        } else {
            insert(attribute, "");
        }
    }
    else {
        std::cerr << "Failed to parse " << ss12000type << "-scim-conf" << std::endl;
    }
    return err;
}

int config_file::load_variables() {
    int err;

    std::string input;

    input = read(filename);

    /* Parse file contents. */
    err = config_parser(std::begin(input), std::end(input)).parse();


    if (err == -1) {
        clear();
        return -1;
    }

    return 0;
}

int config_file::load(const std::string &file_name) {
    filename = filesystem::canonical(file_name);
    int err = load_variables();

    if (!err) {
        load_templates();
    }

    std::string val = get("scim-test-run", true);
    is_test_run = is_true(val);

    return err;
}

std::string config_file::read(const std::experimental::filesystem::path& f) {
    std::string content;

    std::ifstream file(f);
    if (file) {
        std::stringstream buffer;
        // simple rdbuf() read, it's not like it's a large file
        buffer << file.rdbuf();
        content = buffer.str();
        file.close();
    } else {
        simplescim_error_string_set_errno("%s", filename.c_str());
        return "";
    }
    return content;
}

void config_file::clear() {
    vector_cache.clear();
    pair_cache.clear();
    variables.clear();
    filename = "";
}

int config_file::insert(const std::string &variable, const std::string &value) {
    auto iter = variables.find(variable);
    if (iter != variables.end()) {
        iter->second += ", " + value;
    } else {
        variables.emplace(std::make_pair(variable, value));
    }
    return 0;
}

std::vector<std::string> &remove_duplicates(std::vector<std::string> &v) {
    std::set<std::string> clean_vars(v.begin(), v.end());
    v.assign(clean_vars.begin(), clean_vars.end());
    return v;
}

std::vector<std::string>
config_file::get_vector(const std::string &variable, bool silent) const {
    auto cached = vector_cache.find(variable);
    if (cached != vector_cache.end())
        return cached->second;
    else {
        std::vector<std::string> new_vector = string_to_vector(get(variable, silent));
        vector_cache.emplace(std::make_pair(variable, new_vector));
        return new_vector;
    }
}

std::vector<std::string>
config_file::get_vector_sorted_unique(const std::string &variable, bool silent) const {
    std::string s = get(variable, silent);
    std::vector<std::string> v = string_to_vector(s);
    return remove_duplicates(v);
}

std::pair<std::string, std::string>
config_file::get_pair(const std::string &variable, bool silent) const {
    auto cached = pair_cache.find(variable);
    if (cached != pair_cache.end())
        return cached->second;
    else {
        auto tmp = string_to_pair(get(variable, silent));
        pair_cache.emplace(variable, tmp);
        return tmp;
    }
}

const std::string &config_file::get(const std::string &variable, bool silent) const {
    auto val = variables.find(variable);
    if (val != variables.end())
        return val->second;
    else {
        if (variable == "remote_ldap_filter")
            std::cerr << variable << " renamed to ldap_filter. Please change the configuration" << std::endl;
        if (variable == "remote_ldap_base")
            std::cerr << variable << " renamed to ldap_base. Please change the configuration" << std::endl;
        if (!silent) {
            std::cerr << "config_file::get: variable missing: " << variable << std::endl;
            throw std::string("configuration invalid");
        }
    }
    return empty;
}

std::string config_file::interpret_config_path(const std::string& path) const {
    return filesystem::absolute(path, filename.parent_path()).u8string();
}

std::string config_file::get_path(const std::string& variable, bool silent) const {
    auto str = get(variable, silent);
    return interpret_config_path(str);
}

std::vector<std::string> config_file::get_paths(const std::string& variable, bool silent) const {
    auto strs = get_vector(variable, silent);

    for (size_t i = 0; i < strs.size(); ++i) {
        strs[i] = interpret_config_path(strs[i]);
    }
    return strs;
}

bool config_file::has(const std::string& variable) const {
    return variables.find(variable) != variables.end();
}

/**
 * Gets the value associated with 'variable' and stores it
 * in 'valuep' unless 'valuep' is nullptr.
 * If 'variable' has an associated value, zero is returned.
 * Otherwise, -1 is returned and simplescim_error_string is
 * set to an appropriate error message.
 */
std::string config_file::require(const std::string &variable) const {
    const std::string value = get(variable);

    if (value.empty()) {
        simplescim_error_string_set_prefix("simplescim_config_file_require");
        simplescim_error_string_set_message("required variable \"%s\" is missing", variable.c_str());
        return "";
    }

    return value;
}

std::string config_file::require_path(const std::string &variable) const {
    auto str = require(variable);

    if (str.empty()) {
        return "";
    }

    return interpret_config_path(str);
}

//static size_t send_write_func(void *ptr, size_t size, size_t nmemb, void *userdata) {
//    struct http_response *http_response;
//
//    http_response = static_cast<struct http_response *>(userdata);
//    size_t len = size * nmemb;
//
//    for (size_t i = 0; i < len; ++i) {
//        char c = ((char *) ptr)[i];
//
//        if (c == '\r') {
//            continue;
//        }
//
//        if (http_response->len + 1 == http_response->alloc) {
//            char *tmp = static_cast<char *>(realloc(http_response->data, http_response->alloc * 2));
//
//            if (tmp == nullptr) {
//                return i;
//            }
//
//            http_response->data = tmp;
//            http_response->alloc *= 2;
//        }
//
//        http_response->data[http_response->len] = c;
//        ++http_response->len;
//    }
//
//    http_response->data[http_response->len] = '\0';
//
//    return len;
//}
