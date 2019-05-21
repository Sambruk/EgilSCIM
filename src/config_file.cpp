/**
 * Copyright © 2017-2018  Max Wällstedt <>
 *
 * This file is part of EgilSCIM.
 *
 * EgilSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EgilSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with EgilSCIM.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Further development with groups and relations support
 * by Ola Mattsson - IT informa for Sambruk
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

int config_file::load_template(const std::string &ss12000type, const std::string &file) {
    int err = 0;
    std::string content = read(file);

    if (content.empty()) {
        std::cerr << ss12000type << "-scim-conf requested but the file is missing" << std::endl;
        return -1;
    }
    config_parser parser(std::begin(content), std::end(content));
    err = parser.parse();
    if (!err) {
        parser.reset();
        parser.find_variables(ss12000type);
    } else
        std::cerr << "Failed to parse " << ss12000type << "-scim-conf" << std::endl;
    return err;
}

int config_file::load_variables(const std::string &file_name) {
    int err;

    /* Set global string to configuration file's name. */
    filename = file_name;
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
    int err = load_variables(file_name);

    if (!err) {
        load_templates();
    }

    std::string val = get("scim-test-run", true);
    if (!val.empty()) {
        val = toUpper(val);
        if (!val.empty() && val == "TRUE")
            is_test_run = true;
    }

    return err;
}

std::string config_file::read(std::string f) {
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

//void config_file::process_metadata() {
//    std::string md_url("https://fedscim-poc.skolfederation.se/md/skolfederation-fedscim-0_1.json");
//
//    curl_global_init(CURL_GLOBAL_DEFAULT);
//
//    CURL *curl = curl_easy_init();
//    if (curl) {
//        curl_easy_setopt(curl, CURLOPT_URL, md_url.c_str());
//
//        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, send_write_func);
//
//        http_response response;
//
//        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
//
//
//        /* Perform the request, res will get the return code */
//        CURLcode res = curl_easy_perform(curl);
//        /* Check for errors */
//        if (res != CURLE_OK)
//            fprintf(stderr, "curl_easy_perform() failed: %s\n",
//                    curl_easy_strerror(res));
//
//        /* always cleanup */
//        curl_easy_cleanup(curl);
//    }
//
//    curl_global_cleanup();
//
//}
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
