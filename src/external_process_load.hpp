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
 * Parses a JSON array string into an object_list.
 * Each element in the array should be a JSON object with string or
 * array-of-string properties.
 */
std::shared_ptr<object_list> json_to_object_list(const std::string& json,
                                                  const std::string& type);

#endif // EGILSCIMCLIENT_EXTERNAL_PROCESS_LOAD_HPP
