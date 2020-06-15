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

#ifndef EGILSCIM_PP_INTERFACE_H
#define EGILSCIM_PP_INTERFACE_H

/*
 * The constants below are used as return values in the plugin's
 * include function, to determine how to deal with different object types.
 */

// The plugin blocks the type from being sent
#define PP_BLOCK_TYPE   0
// The plugin should process this type
#define PP_PROCESS_TYPE 1
// The plugin shouldn't process this type, but it should still be sent
#define PP_SKIP_TYPE    2

/*
 * Below are definitions of function pointers to the functions the plugin
 * is expected to export.
 */

/*
 * The init function gets all configuration variables for the plugin.
 * In other words, if the plugin is named foo, all config variables
 * starting with pp-foo- will be included.
 */
typedef int (*pp_plugin_init_func)(int count, char** vars, char** values);

/*
 * The include function lets the plugin decide how to deal with each
 * object type. The constants defined above should be used as return
 * value.
 */
typedef int (*pp_plugin_include_func)(const char* type);

/*
 * The process function does the actual post processing. Each call
 * is for one object.
 *
 * The plugin shall return 0 on success. On failure, it shall return
 * non-zero and send an error message through output. The error message 
 * will be free'd with the free function just like regular output.
 */
typedef int (*pp_plugin_process_func)(const char* type,
                                      const char* input,
                                      char** output);

/*
 * Once the EGIL client is done with the output from the process function
 * it will be sent to the free function so the plugin can de-allocate the memory.
 */
typedef void (*pp_plugin_free_func)(void* ptr);

/*
 * The exit function is called at the end of the plugin life time.
 */
typedef void (*pp_plugin_exit_func)();

#endif // EGILSCIM_PP_INTERFACE_H

