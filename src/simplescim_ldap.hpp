/**
 * Copyright © 2017-2018  Max Wällstedt <>
 *
 * This file is part of SimpleSCIM.
 *
 * SimpleSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with SimpleSCIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SIMPLESCIM_LDAP_H
#define SIMPLESCIM_LDAP_H

#include <memory>
#include "ldap_wrapper.hpp"

class object_list;

/**
 * Reads user data from LDAP into a user list object
 * according to the configuration file and returns a
 * pointer it. On error, NULL is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
std::shared_ptr<object_list> ldap_get(ldap_wrapper &ldap, const std::string &type);
void load_related(const std::string &type, const std::shared_ptr<object_list> &objects);

#endif
