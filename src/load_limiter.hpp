/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2024 Föreningen Sambruk
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

#ifndef EGILSCIM_LOAD_LIMITER_HPP
#define EGILSCIM_LOAD_LIMITER_HPP

#include <string>
#include "model/base_object.hpp"

/** 
 * The load_limiter is used to restrict which objects to load.
 * 
 * Depending on the way a type is limited in the config file, we'll
 * instantiate different sub-classes.
 */
class load_limiter {
public:
    virtual ~load_limiter() {}
    virtual bool include(const base_object* obj) const = 0;
};

std::shared_ptr<load_limiter> get_limiter(const std::string& type);

/**
 * Sets a user blacklist. The blacklist works like a list limiter,
 * so the blacklisted users are listed in a file. If an attribute
 * is given, the values in the file are matched against that attribute
 * in the data source, otherwise the values are assumed to be UUIDs.
 * 
 * If a blacklist is used, it will be combined with the limiters
 * returned by get_limiter (if called for a type that has "Users" as
 * the endpoint), so that the returned limiter excludes users from
 * the blacklist.
 */
void set_user_blacklist(const std::string &filename,
                        const std::string &attrib);

#endif // EGILSCIM_LOAD_LIMITER_HPP
