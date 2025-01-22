/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2025 Föreningen Sambruk
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

#ifndef EGILSCIM_PRINT_CACHE_HPP
#define EGILSCIM_PRINT_CACHE_HPP

#include <memory>
#include <vector>
#include <string>
#include "model/rendered_object_list.hpp"

/** Prints certain objects from the cache to stdout.
 *  types can be used to limit the objects to certain types, if types is empty
 *  objects of all types are printed.
 *  If by_endpoint is true the types are interpreted as SCIM endpoints instead
 *  of EGIL types.
 *  where contains conditions that must be met for each object to print, in the
 *  format attribute=value. For instance userName=babs@example.com . If there
 *  are multiple where conditions all must be met.
 */
void print_cache(std::shared_ptr<rendered_object_list> cache,
                 bool by_endpoint,
                 const std::vector<std::string> &types,
                 const std::vector<std::string> &where);

#endif // EGILSCIM_PRINT_CACHE_HPP
