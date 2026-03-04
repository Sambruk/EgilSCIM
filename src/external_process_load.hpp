/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2026 Föreningen Sambruk
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

#ifndef EGILSCIMCLIENT_EXTERNAL_PROCESS_LOAD_HPP
#define EGILSCIMCLIENT_EXTERNAL_PROCESS_LOAD_HPP

#include <memory>
#include <model/object_list.hpp>
#include "external_process.hpp"
#include "utility/indented_logger.hpp"

/**
 *  external_process_get reads objects for a type by running an
 *  external process. It's analogous to csv_get and sql_get.
 */
std::shared_ptr<object_list> external_process_get(const external_process_manager& manager,
                                                   const std::string &type,
                                                   indented_logger& load_logger);

/**
 * A sink that receives individual JSON object strings, parses each one
 * into a base_object, and adds it to an object_list.
 *
 * Designed to be used as the inner sink of a json_array_splitter, so
 * that each write() call receives exactly one complete JSON object.
 *
 * On parse errors or duplicate UUIDs, sets a failure flag and stores
 * an error message instead of throwing. Once failed, ignores
 * subsequent objects.
 */
class json_parser_sink : public process_sink {
public:
    json_parser_sink(std::shared_ptr<object_list> objects, const std::string& type);
    void write(const char* data, size_t len) override;
    bool failed() const { return failed_; }
    const std::string& error_message() const { return error_message_; }

private:
    std::shared_ptr<object_list> objects_;
    std::string type_;
    bool ignore_dups_;
    bool failed_ = false;
    std::string error_message_;
};

/**
 * A sink that splits a stream of bytes containing a JSON array of objects
 * into individual JSON objects, forwarding each complete object to an
 * inner sink.
 *
 * Expects the input to be a JSON array (starting with '[', ending with ']').
 * Uses brace counting to find object boundaries, tracking whether
 * the parser is inside a string to avoid miscounting braces within
 * string values. Handles backslash escapes inside strings.
 *
 * If the input is not a valid array of objects, the splitter enters
 * a failed state and ignores all subsequent data.
 */
class json_array_splitter : public process_sink {
public:
    explicit json_array_splitter(process_sink& inner);
    void write(const char* data, size_t len) override;
    bool failed() const { return failed_; }

private:
    process_sink& inner_;
    bool failed_ = false;
    bool seen_array_start_ = false;
    bool done_ = false;
    bool in_string_ = false;
    bool escape_next_ = false;
    int brace_depth_ = 0;
    std::string current_object_;
};

#endif // EGILSCIMCLIENT_EXTERNAL_PROCESS_LOAD_HPP
