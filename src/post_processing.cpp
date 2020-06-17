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
#include <experimental/filesystem>

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include "config_file.hpp"

namespace post_processing {

// Config parameters for a plugin
struct init_args {
    int count;
    char** vars;
    char** values;
};

// Gets all config parameters for a plugin from the config file.
init_args get_init_args(const std::string& plugin_name) {
    init_args result;

    auto prefix = std::string("pp-" + plugin_name);

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

// Releases resources for init_args
void free_init_args(const init_args& args) {
    for (int i = 0; i < args.count; ++i) {
        delete[] args.vars[i];
        delete[] args.values[i];
    }
    delete[] args.vars;
    delete[] args.values;
}

std::string get_dl_error() {
#ifdef _WIN32
    LPVOID lpMsgBuf;
    DWORD dw = GetLastError();
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        dw,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&lpMsgBuf,
        0, NULL);
    std::string result((LPSTR)lpMsgBuf);
    LocalFree(lpMsgBuf);
    return result;
#else
    return dlerror();
#endif
}

/*
 * Convenience function for finding a symbol from a shared library.
 * Throws runtime_error on failure.
 */
void* plugin::find_func(std::string symbol_name) {
#ifdef _WIN32
    auto func = GetProcAddress(lib_handle, symbol_name.c_str());
#else
    auto func = dlsym(lib_handle, symbol_name.c_str());
#endif

    if (func == nullptr) {
        std::string err(get_dl_error());
        throw std::runtime_error(std::string("Plugin ") + plugin_name + " is missing a function (" + symbol_name + ")");
    }
    return func;
}

// Returns the expected full path to a plugin.
std::string library_path(const std::string& path, const std::string& plugin_name) {
    auto result = std::experimental::filesystem::path(path);
#ifdef _WIN32
    result.append(plugin_name + ".dll");
#else
    result.append(std::string("lib") + plugin_name + ".so");
#endif
    return result.string();
}

plugin::plugin(const std::string& path, const std::string& p_name)
        : plugin_name(p_name) {
    const auto full_path = library_path(path, plugin_name);

#ifdef _WIN32
    lib_handle = LoadLibrary(full_path.c_str());
#else
    lib_handle = dlopen(full_path.c_str(), RTLD_NOW);
#endif

    if (lib_handle == nullptr) {
        std::string err(get_dl_error());
        throw std::runtime_error(std::string("Failed to load plugin " + plugin_name + " : " + err));
    }

    auto init_func    = (pp_plugin_init_func)   find_func(plugin_name + "_init");
    include_func      = (pp_plugin_include_func)find_func(plugin_name + "_include");
    process_func      = (pp_plugin_process_func)find_func(plugin_name + "_process");
    free_func         = (pp_plugin_free_func)   find_func(plugin_name + "_free");
    exit_func         = (pp_plugin_exit_func)   find_func(plugin_name + "_exit");

    auto args = get_init_args(plugin_name);
    char* error = nullptr;
    int err = init_func(args.count, args.vars, args.values, &error);
    std::string strerr;
    if (error != nullptr) {
        strerr = error;
        free_func(error);
    }

    if (err != 0) {
        throw std::runtime_error(std::string("Failed to initialize plugin + ") + plugin_name + " (code: " + std::to_string(err) + ", message: " + strerr + ")");
    }

    free_init_args(args);
}

plugin::~plugin() {
    if (lib_handle != nullptr) {
        exit_func();
#ifdef _WIN32
        FreeLibrary(lib_handle);
#else
        dlclose(lib_handle);
#endif
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
