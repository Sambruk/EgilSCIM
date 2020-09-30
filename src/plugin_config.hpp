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

#ifndef EGILSCIM_PLUGIN_CONFIG_HPP
#define EGILSCIM_PLUGIN_CONFIG_HPP

#include <string>

/*
 * Functionality for passing config parameters to plugins.
 *
 * So basically getting all config parameters with a given
 * prefix and placing them in memory allocated with raw
 * pointers so we can send them to a library with a C API.
 */
 
// Config parameters for a plugin
struct init_args {
    int count;
    char** vars;
    char** values;
};

// Gets all config parameters for a plugin from the config file.
init_args get_init_args(const std::string& plugin_type_prefix, const std::string& plugin_name);

// Releases resources for init_args
void free_init_args(const init_args& args);

#endif // EGILSCIM_PLUGIN_CONFIG_HPP
