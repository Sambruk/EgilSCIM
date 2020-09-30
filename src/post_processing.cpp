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

#include "post_processing.hpp"

#include "plugin_config.hpp"

namespace post_processing {

plugin::plugin(const std::string& path, const std::string& p_name)
        : plugin_name(p_name) {
    try {
        lib_handle = dl_load(path, plugin_name);
    }
    catch (const std::runtime_error& e) {
        throw std::runtime_error(std::string("Failed to load plugin " + plugin_name + " : " + e.what()));
    }

    pp_plugin_init_func init_func;
    try {
        init_func = (pp_plugin_init_func)find_func(lib_handle, plugin_name + "_init");
        include_func = (pp_plugin_include_func)find_func(lib_handle, plugin_name + "_include");
        process_func = (pp_plugin_process_func)find_func(lib_handle, plugin_name + "_process");
        free_func = (pp_plugin_free_func)find_func(lib_handle, plugin_name + "_free");
        exit_func = (pp_plugin_exit_func)find_func(lib_handle, plugin_name + "_exit");
    }
    catch (const std::runtime_error &e) {
        throw std::runtime_error(std::string("Failed to load function from plugin " + plugin_name + "(" + e.what() + ")"));
    }

    auto args = get_init_args("pp-", plugin_name);
    char* error = nullptr;
    int err = init_func(args.count, args.vars, args.values, &error);
    std::string strerr;
    if (error != nullptr) {
        strerr = error;
        free_func(error);
    }

    if (err != 0) {
        throw std::runtime_error(std::string("Failed to initialize plugin ") + plugin_name + " (code: " + std::to_string(err) + ", message: " + strerr + ")");
    }

    free_init_args(args);
}

plugin::~plugin() {
    if (lib_handle != nullptr) {
        exit_func();
        dl_free(lib_handle);
    }
}

int plugin::include(const std::string& type) {
    return include_func(type.c_str());
}

std::string plugin::process(const std::string& type, const std::string& input) {
    char* output;
    int result = process_func(type.c_str(), input.c_str(), &output);
    std::string str(output);
    free_func(output);

    if (result != 0) {
        std::string error{"failed to process object of type "};
        error += type + " in plugin " + plugin_name;
        error += " (error code: " + std::to_string(result) + ")";
        throw std::runtime_error(error);
    }
    
    return str;
}

plugins load_plugins(const std::string& path, const std::vector<std::string>& plugin_names) {
    plugins result;

    for (const auto& name : plugin_names) {
        result.push_back(std::make_shared<plugin>(path, name));
    }
    return result;
}

std::vector<std::string> filter_types(const std::vector<std::string>& types, const plugins& ppp) {
    std::vector<std::string> result;

    for (const auto& type : types) {
        bool block = false;
        for (const auto& plugin : ppp) {
            if (plugin->include(type) == PP_BLOCK_TYPE) {
                block = true;
                break;
            }
        }
        if (!block) {
            result.push_back(type);
        }
    }

    return result;
}

std::string process(const plugins& ppp,const std::string& type, const std::string& input) {
    auto next = input;
    for (auto plugin : ppp) {
        if (plugin->include(type) == PP_PROCESS_TYPE) {
            next = plugin->process(type, next);
        }
    }
    return next;
}

}
