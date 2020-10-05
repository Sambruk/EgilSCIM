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

#ifndef EGILSCIM_SQL_HPP
#define EGILSCIM_SQL_HPP

#include <string>
#include <memory>
#include <vector>
#include <optional>
#include "dl.hpp"
#include "sql_interface.h"

namespace sql {

/*
 * The plugin class loads and represents one SQL plugin.
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

    // The iterator is used to iterate through results row by row.
    class iterator {
    public:
        ~iterator();

        /** 
         * Returns true if another row was fetched, false if we've
         * reached the end.
         * 
         * Throws std::runtime_error on a failure from the SQL plugin.
         */
        bool next(std::vector<std::optional<std::string>>& row);

        // Gets the list of column names for the current result        
        std::vector<std::string> get_header() { return header; }

    private:
        iterator(SQL_PLUGIN_CURSOR c,
                 sql_plugin_header_func header_func,
                 sql_plugin_next_row_func nr,
                 sql_plugin_free_cursor fc) : cursor(c), next_row_func(nr), free_func(fc) {
            save_header(header_func);
        }

        iterator() = delete;
        iterator(const iterator&) = delete;

        friend plugin;

        void save_header(sql_plugin_header_func header_func);

        std::vector<std::string> header;
        SQL_PLUGIN_CURSOR cursor;
        sql_plugin_next_row_func next_row_func;
        sql_plugin_free_cursor free_func;
    };

    /*
     * Executes an SQL query.
     * Throws runtime_error if the plugin returns an error.
     */
    std::shared_ptr<iterator> execute(const std::string& query);

private:
    dl_handle lib_handle = DL_NULL;

    std::string plugin_name;

    sql_plugin_execute_func execute_func;
    sql_plugin_header_func header_func;
    sql_plugin_next_row_func next_row_func;
    sql_plugin_free_cursor free_cursor_func;
    sql_plugin_free_error free_error_func;
    sql_plugin_exit_func exit_func;
};

}

#endif // EGILSCIM_SQL_HPP