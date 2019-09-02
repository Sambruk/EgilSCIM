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

#ifndef SIMPLESCIM_LDAP_H
#define SIMPLESCIM_LDAP_H

#include <memory>
#include "ldap_wrapper.hpp"
#include "utility/indented_logger.hpp"

class object_list;

/**
 * Reads user data from LDAP into a user list object
 * according to the configuration file and returns a
 * pointer it. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
std::shared_ptr<object_list> ldap_get(ldap_wrapper &ldap,
                                      const std::string &type,
                                      indented_logger& load_logger);

void load_related(const std::string &type,
                  const std::shared_ptr<object_list> &objects,
                  indented_logger& load_logger);

#endif
