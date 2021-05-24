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

#include "rendered_object_list.hpp"

void rendered_object_list::add_object(std::shared_ptr<rendered_object> obj) {
    objects[obj->get_id()] = obj;
}

std::shared_ptr<rendered_object> rendered_object_list::get_object(const std::string &id) {
    auto record = objects.find(id);
    if (record != objects.end()) {
        return record->second;
    }
    return nullptr;
}
