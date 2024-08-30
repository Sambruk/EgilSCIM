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

#include "dl.hpp"

#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
#include <experimental/filesystem>

namespace {

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

}

void* find_func(dl_handle lib_handle, std::string symbol_name) {
#ifdef _WIN32
    auto func = GetProcAddress(lib_handle, symbol_name.c_str());
#else
    auto func = dlsym(lib_handle, symbol_name.c_str());
#endif

    if (func == nullptr) {
        throw std::runtime_error(symbol_name + " : " + get_dl_error());
    }
    return func;
}

dl_handle dl_load(const std::string& path, const std::string& plugin_name) {
    const auto full_path = library_path(path, plugin_name);

    dl_handle res = 
#ifdef _WIN32
        LoadLibrary(full_path.c_str());
#else
        dlopen(full_path.c_str(), RTLD_NOW);
#endif
    if (res == DL_NULL) {
        throw std::runtime_error(get_dl_error());
    }
    return res;
}

void dl_free(dl_handle lib_handle) {
#ifdef _WIN32
    FreeLibrary(lib_handle);
#else
    dlclose(lib_handle);
#endif
}