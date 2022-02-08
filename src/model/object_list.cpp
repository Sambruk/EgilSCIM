/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2022 Föreningen Sambruk
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

#include "object_list.hpp"

std::shared_ptr<base_object> object_list::get_object_for_attribute(const std::string &attribute, const std::string &id){
    for (const auto &object : objects) {
        const string_vector &values = object.second->get_values(attribute);
        for (auto &&value : values) {
            if (value == id)
                return object.second;
        }
    }
    return nullptr;
}

void object_list::add_object(const std::string &uid, std::shared_ptr<base_object> object) {
    objects[uid] = object;
}

void object_list::remove(const std::string &uuid) { 
    objects.erase(uuid); 
}

object_list &object_list::operator+=(const object_list &other) {
  for (auto &&object : other.objects) {
    objects.emplace(object.first, object.second);
  }
  return *this;
}