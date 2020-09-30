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
#ifndef EGILSCIMCLIENT_SQL_LOAD_HPP
#define EGILSCIMCLIENT_SQL_LOAD_HPP

#include <memory>
#include <model/object_list.hpp>
#include "sql.hpp"
#include "utility/indented_logger.hpp"

/**
 *  sql_get reads objects for a type from a SQL source. It's analoguous
 *  to ldap_get.
 */
std::shared_ptr<object_list> sql_get(std::shared_ptr<sql::plugin> plugin,
                                     const std::string &type,
                                     indented_logger &load_logger);

#endif // EGILSCIMCLIENT_SQL_LOAD_HPP
