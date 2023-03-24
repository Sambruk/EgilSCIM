/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2023 Föreningen Sambruk
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

#ifndef READABLE_ID_HPP
#define READABLE_ID_HPP

#include "model/base_object.hpp"
#include <string>

// Returns a human readable identifier for the object if possible
// In general we can only assume to know the objects UUID, which isn't very
// helpful in error messages etc.
// If there is a configuration variable specifying an attribute to use for
// the type in question we'll use that (UUID is also returned).
// If there's no configured variable we'll try cn since it's usually available
// if the source is LDAP.
// If no suitable attribute can be found we'll just return the UUID.
std::string readable_id(const base_object* obj, const std::string type = "");

#endif // READABLE_ID_HPP
