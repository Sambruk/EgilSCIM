/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2019 Föreningen Sambruk
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

#ifndef EGILSCIMCLIENT_GENERATED_LOAD_HPP
#define EGILSCIMCLIENT_GENERATED_LOAD_HPP

#include <memory>
#include <model/object_list.hpp>
#include "sql.hpp"
#include "utility/indented_logger.hpp"

std::shared_ptr<object_list> get_generated(const std::string& type,
                                           std::shared_ptr<sql::plugin> sql_plugin,
                                           indented_logger& load_logger);
#endif  // EGILSCIMCLIENT_GENERATED_LOAD_HPP
