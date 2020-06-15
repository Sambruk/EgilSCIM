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

#ifndef EGILSCIM_POST_PROCESSING_HPP
#define EGILSCIM_POST_PROCESSING_HPP

#include <vector>
#include <memory>
#include "pp_interface.h"

namespace post_processing {

/*
 * The plugin class loads and represents one post processing plugin.
 *
 * The shared library is dynamically loaded when the plugin class is
 * instantiated, and closed when the plugin object is destroyed.
 */
class plugin {
public:
    // Throws a runtime_error exception if the plugin failed to load
    plugin(const std::string& path, const std::string& plugin_name);

    // Releases the shared library
    ~plugin();

    // Checks if and how to process a type
    int include(const std::string& type);

    /*
     * Does post processing of one object of a given type.
     * Throws runtime_error if the plugin returns an error.
     */
    std::string process(const std::string& type, const std::string& input);

private:
    void* find_func(std::string symbol_name);
        
    void* lib_handle = nullptr;
    std::string plugin_name;

    pp_plugin_include_func include_func;
    pp_plugin_process_func process_func;
    pp_plugin_free_func free_func;
    pp_plugin_exit_func exit_func;
};

// A sequence of plugins
using plugins = std::vector<std::shared_ptr<plugin>>;

// Loads a sequence of plugins
plugins load_plugins(const std::string& path, const std::vector<std::string>& plugin_names);

// Filters out the types that at least one of the plugins wishes to block
std::vector<std::string> filter_types(const std::vector<std::string>& types, const plugins& ppp);

/*
 * Does the post processing for one object according to plugins, in the order
 * the plugins appear in the sequence.
 *
 * Only plugins that want to process the type in question will be called.
 *
 * If one plugin fails the whole function will throw runtime_error.
 */
std::string process(const plugins& ppp, const std::string& type, const std::string& input);

} // namespace post_processing

#endif // EGILSCIM_POST_PROCESSING_HPP
