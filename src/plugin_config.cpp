/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2020 Föreningen Sambruk
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

#include "plugin_config.hpp"
#include <vector>
#include "config_file.hpp"

init_args get_init_args(const std::string& plugin_type_prefix, const std::string& plugin_name) {
    init_args result;

    auto prefix = std::string(plugin_type_prefix + plugin_name);

    std::vector<std::string> vars, values;
    
    for (auto itr = config_file::instance().begin();
         itr != config_file::instance().end();
         ++itr) {
        auto var = itr->first;
        auto value = itr->second;

        if (startsWith(var, prefix)) {
            vars.push_back(var);
            values.push_back(value);
        }
    }

    result.count = static_cast<int>(vars.size());
    result.vars = new char*[vars.size()];
    result.values = new char*[vars.size()];

    for (size_t i = 0; i < vars.size(); ++i) {
        auto var = vars[i];
        auto value = values[i];

        result.vars[i] = new char[var.size()+1];
        strncpy(result.vars[i], var.c_str(), var.size()+1);

        result.values[i] = new char[value.size()+1];
        strncpy(result.values[i], value.c_str(), value.size()+1);
    }
    
    return result;
}

void free_init_args(const init_args& args) {
    for (int i = 0; i < args.count; ++i) {
        delete[] args.vars[i];
        delete[] args.values[i];
    }
    delete[] args.vars;
    delete[] args.values;
}
