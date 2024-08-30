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

#ifndef SIMPLESCIM_CONFIG_FILE_H
#define SIMPLESCIM_CONFIG_FILE_H

#include <string>
#include <iostream>
#include <map>
#include "utility/utils.hpp"
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

class config_file {
    // Absolute, canonical path to the config file
    std::experimental::filesystem::path filename;

    std::map<std::string, std::string> variables{};

    // caches
    mutable std::map< std::string, std::vector<std::string> > vector_cache;
    mutable std::map< std::string, std::pair<std::string, std::string> > pair_cache;

    const std::string empty{};
    bool is_test_run = false;

    config_file() = default;

    config_file(const config_file &other) = default;

    std::string read(const std::experimental::filesystem::path& path);

    int load_variables();

    int load_templates();

    int load_template(const std::string &ss12000type, const std::string &filename);

public:
    static config_file &instance() {
        static config_file theconfigfile;
        return theconfigfile;
    }

    std::string file_name_str() {
        return filename.u8string();
    }

    /**
     * Relative path names given in the config file should be considered
     * relative to the config file's directory. This function converts
     * a path to an absolute path relative to the config file's directory.
     * 
     * Normally this shouldn't be used from outside of the config_file
     * class, see for instange get_path() instead.
     */
    std::string interpret_config_path(const std::string& path) const;

    int load(const std::string &file_name);

    bool test_run() const {
        return is_test_run;
    }

    void clear();

    int insert(const std::string &variable, const std::string &value);

    const std::string &get(const std::string &variable, bool silent = false) const;

    std::vector<std::string> get_vector(const std::string &variable, bool silent = false) const;

    std::vector<std::string> get_vector_sorted_unique(const std::string &variable, bool silent = false) const;

    std::pair<std::string, std::string> get_pair(const std::string &variable, bool silent = false) const;

    bool get_bool(const std::string &attrib) const {
        return is_true(get(attrib, true));
    }

    std::string get_path(const std::string& variable, bool silent = false) const;

    std::vector<std::string> get_paths(const std::string& variable, bool silent = false) const;

    bool has(const std::string& variable) const;

    std::string require(const std::string &variable) const;
    
    std::string require_path(const std::string &variable) const;

    std::map<std::string, std::string>::const_iterator begin() const { return std::begin(variables); }

    std::map<std::string, std::string>::const_iterator end() const { return std::end(variables); }

    void add_variable(const std::string &attrib, const std::string &var) {
        auto old = variables.find(attrib);
        if (old == variables.end()) {
            insert(attrib, var);
        } else {
            old->second += ", " + var;
        }
    }
    void replace_variable(const std::string &attrib, const std::string &var) {
        auto iter = variables.find(attrib);
        if (iter != variables.end()) {
            iter->second = var;
        } else {
            variables.emplace(std::make_pair(attrib, var));
        }
    }
};

#endif
