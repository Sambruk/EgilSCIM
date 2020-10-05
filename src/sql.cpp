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

#include "sql.hpp"
#include "plugin_config.hpp"

namespace sql {

plugin::iterator::~iterator() {
    if (cursor != nullptr) {
        free_func(cursor);
    }
}

void plugin::iterator::save_header(sql_plugin_header_func header_func) {
    int column_count;
    char** column_names;
    auto res = header_func(cursor, &column_count, &column_names);

    if (res != SQL_PLUGIN_SUCCESS) {
        throw std::runtime_error("Failed to retrieve header for SQL results");
    }

    header.resize(column_count);
    for (int i = 0; i < column_count; ++i) {
        header[i] = column_names[i];
    }
}

bool plugin::iterator::next(std::vector<std::optional<std::string>>& row) {
    if (cursor == nullptr) {
        return false;
    }

    char** values;
    auto res = next_row_func(cursor, &values);

    if (res == SQL_PLUGIN_END) {
        free_func(cursor);
        cursor = nullptr;
        return false;
    } else if (res == SQL_PLUGIN_SUCCESS) {
        row.clear(); // make sure it's all null when we resize
        row.resize(header.size());
        for (size_t i = 0; i < header.size(); ++i) {
            if (values[i] != nullptr) {
                row[i] = values[i];
            }
        }
        return true;
    } else {
        throw std::runtime_error("Failed to advance SQL cursor");
    }
}

plugin::plugin(const std::string &path, const std::string &p_name)
    : plugin_name(p_name) {
    try {
        lib_handle = dl_load(path, plugin_name);
    }
    catch (const std::runtime_error& e) {
        throw std::runtime_error(std::string("Failed to load plugin " + plugin_name + " : " + e.what()));
    }

    sql_plugin_init_func init_func;
    try {
        init_func = (sql_plugin_init_func)find_func(lib_handle, plugin_name + "_init");
        execute_func = (sql_plugin_execute_func)find_func(lib_handle, plugin_name + "_execute");
        header_func = (sql_plugin_header_func)find_func(lib_handle, plugin_name + "_header");
        next_row_func = (sql_plugin_next_row_func)find_func(lib_handle, plugin_name + "_next_row");
        free_cursor_func = (sql_plugin_free_cursor)find_func(lib_handle, plugin_name + "_free_cursor");
        free_error_func = (sql_plugin_free_error)find_func(lib_handle, plugin_name + "_free_error");
        exit_func = (sql_plugin_exit_func)find_func(lib_handle, plugin_name + "_exit");
    }
    catch (const std::runtime_error &e) {
        throw std::runtime_error(std::string("Failed to load function from plugin " + plugin_name + "(" + e.what() + ")"));
    }

    auto args = get_init_args("sql-", plugin_name);
    char* error = nullptr;
    int err = init_func(args.count, args.vars, args.values, &error);
    std::string strerr;
    if (error != nullptr) {
        strerr = error;
        free_error_func(error);
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

std::shared_ptr<plugin::iterator> plugin::execute(const std::string& query) {
    SQL_PLUGIN_CURSOR cursor;
    char* error;
    auto res = execute_func(query.c_str(), &cursor, &error);

    if (res != SQL_PLUGIN_SUCCESS) {
        std::string message = std::string("Failed to execute SQL query: ") + error;
        free_error_func(error);
        throw std::runtime_error(message);

    }

    return std::shared_ptr<iterator>(new iterator(cursor, header_func, next_row_func, free_cursor_func));
}

}