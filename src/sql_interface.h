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

#ifndef EGILSCIM_SQL_INTERFACE_H
#define EGILSCIM_SQL_INTERFACE_H

#define SQL_PLUGIN_SUCCESS 0
#define SQL_PLUGIN_END 1

/*
 * Below are definitions of function pointers to the functions the plugin
 * is expected to export.
 */

/*
 * The init function gets all configuration variables for the plugin.
 * In other words, if the plugin is named foo, all config variables
 * starting with sql-foo- will be included.
 *
 * The function shall initialize the plugin including connecting to
 * the SQL database and authenticating.
 * 
 * If the initialization failed, the plugin shall return non-zero
 * and set an error message in the error parameter. The error message
 * should be free'd with the plugin's free_error function.
 */
typedef int (*sql_plugin_init_func)(int count,
                                   char** vars,
                                   char** values,
                                   char** error);

/*
 * An opaque type used to iterate through the rows returned by
 * an SQL query.
 */
typedef void* SQL_PLUGIN_CURSOR;

/*
 * The execute function executes an SQL query.
 * 
 * If all goes well SQL_PLUGIN_SUCCESS is returned and cursor
 * is set to a new cursor.
 * 
 * If an error occurs a return value != SQL_PLUGIN_SUCCESS is
 * returned and error is set to a message explaining the error.
 *
 * The error string should be released with the free_error
 * function.
 * 
 * sql_statement is in UTF-8.
 */
typedef int (*sql_plugin_execute_func)(const char *sql_statement,
                                       SQL_PLUGIN_CURSOR *cursor,
                                       char **error);

/*
 * The header function is called after successful execution of
 * a query in order to get information about the number of columns
 * and their names.
 * 
 * The column names need not be free'd, but should not be used
 * after the cursor is released.
 * 
 * The column_names are returned in UTF-8.
 */
typedef int (*sql_plugin_header_func)(SQL_PLUGIN_CURSOR cursor,
                                      int *column_count,
                                      char ***column_names);

/*
 * The next function returns the next row of a result.
 * 
 * The values need not be free'd, but should not be used
 * after next is called again for this cursor.
 * 
 * The values are returned in UTF-8.
 * 
 * If the function is called after the last row has been returned,
 * the function returns SQL_PLUGIN_END (and does not modify values).
 */
typedef int (*sql_plugin_next_row_func)(SQL_PLUGIN_CURSOR cursor,
                                        char ***values);

/*
 * The free_cursor function is called when the result set 
 * is not needed anymore, typically after iterating through all rows.
 */
typedef void (*sql_plugin_free_cursor)(SQL_PLUGIN_CURSOR cursor);

/*
 * The free_error message is called to release memory for an error 
 * message.
 */
typedef void (*sql_plugin_free_error)(char* error);

/*
 * The exit function is called at the end of the plugin life time.
 *
 * The exit function shall disconnect from the database and clean
 * up any used resources.
 */
typedef void (*sql_plugin_exit_func)();

#endif // EGILSCIM_SQL_INTERFACE_H
