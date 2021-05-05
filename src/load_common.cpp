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

#include "load_common.hpp"
#include <cassert>

std::shared_ptr<object_list> filter_objects(std::shared_ptr<object_list> objects,
                                            std::shared_ptr<load_limiter> limiter,
                                            indented_logger &load_logger,
                                            const std::string& type) {
    auto included_objects = std::make_shared<object_list>();

    for (auto &iter : *objects) {
        auto object = iter.second;
        auto uid = object->get_uid();
        assert(!uid.empty());
        if (limiter->include(object.get())) {
            included_objects->add_object(uid, object);
            load_logger.log("Found " + type + " " + uid);
        }
    }

    return included_objects;
}
