/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2021 Föreningen Sambruk
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

#ifndef EGILSCIM_LOAD_COMMON_HPP
#define EGILSCIM_LOAD_COMMON_HPP

#include <memory>
#include "model/object_list.hpp"
#include "transformer.hpp"
#include "load_limiter.hpp"
#include "utility/indented_logger.hpp"

void load_related(const std::string &type,
                  const std::shared_ptr<object_list> &objects,
                  indented_logger& load_logger);
/**
 * Applies a transformer to all objects in a list.
 */
void transform_objects(std::shared_ptr<object_list> objects, std::shared_ptr<transformer> transform);

/**
 * Filters out only the objects which the load limiter thinks should be included.
 * The included objects are added to a new list which is returned. For each
 * included object we log it to the load logger.
 *
 * We assume all objects have proper uid attributes.
 */
std::shared_ptr<object_list> filter_objects(std::shared_ptr<object_list> objects,
                                            std::shared_ptr<load_limiter> limiter,
                                            indented_logger &load_logger,
                                            const std::string& type);

void establish_relation(std::shared_ptr<base_object> main_object,
                        std::shared_ptr<base_object> remote,
                        const std::string& main_type,
                        const std::string& remote_type);

/**
 * Treatment of bad UUIDs. This function will print
 * a warning to stderr if the uuid is bad and return whether
 * or not the object should be allowed to be loaded.
 * 
 * By default all objects are allowed to be loaded regardless
 * of what their UUID looks like (for backwards compatibility
 * reasons), but this can be changed with a config variable.
 * 
 * The warning can also be disabled by a config variable.
*/
bool warn_if_bad_uuid(const std::string& uuid);

#endif // EGILSCIM_LOAD_COMMON_HPP
