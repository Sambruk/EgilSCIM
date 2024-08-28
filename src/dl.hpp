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

#ifndef EGILSCIM_DL_HPP
#define EGILSCIM_DL_HPP

/*
 * This file contains stuff for working with dynamically loaded libraries
 * in a cross platform way (Windows and Linux).
 */

#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

// dl_handle refers to a dynamically loaded library
#ifdef _WIN32
typedef HINSTANCE dl_handle;
#else
typedef void* dl_handle;
#endif

// The null handle
#define DL_NULL nullptr

/*
 * Loads a shared library.
 * 
 * path is a directory, plugin_name is the name without platform
 * specific prefix/suffix.
 * 
 * For instance, if plugin_name is foo we'll try to load libfoo.so
 * on Linux, and foo.dll on Windows.
 */
dl_handle dl_load(const std::string& path, const std::string& plugin_name);

// Releases a dynamically loaded library
void dl_free(dl_handle lib_handle);

/*
 * Finds a symbol from a shared library.
 * Throws runtime_error on failure.
 */
void* find_func(dl_handle lib_handle, std::string symbol_name);

#endif // EGILSCIM_DL_HPP
