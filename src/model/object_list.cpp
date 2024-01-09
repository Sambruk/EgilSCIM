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

std::vector<std::shared_ptr<base_object>> object_index::lookup(const std::string &value) {
    auto itr = idx.find(value);
    if (itr != idx.end()) {
        return itr->second;
    } else {
        return std::vector<std::shared_ptr<base_object>>();
    }
}

void object_index::add(std::shared_ptr<base_object> object) {
    auto values = object->get_values(attribute);
    for (const auto &v : values) {
        idx[v].push_back(object);
    }
}

void object_index::remove(std::shared_ptr<base_object> object) {
    auto values = object->get_values(attribute);
    for (const auto &v : values) {
        auto &objs = idx[v];
        auto itr = objs.begin();
        while (itr != objs.end()) {
            if ((*itr).get() == object.get()) {
                itr = objs.erase(itr);
            } else {
                ++itr;
            }
        }
    }
}

std::vector<std::shared_ptr<base_object>> object_list::get_objects_for_attribute(const std::string &attribute, const std::string &value) {
    auto idx = find_index(attribute);

    if (idx == nullptr) {
        idx = std::make_shared<object_index>(attribute);

        for (const auto& itr : objects) {
            idx->add(itr.second);
        }

        indices.push_back(idx);
    }

    return idx->lookup(value);
}

std::shared_ptr<base_object> object_list::get_object_for_attribute(const std::string &attribute, const std::string &id) {
    auto matches = get_objects_for_attribute(attribute, id);

    if (matches.empty()) {
        return nullptr;
    }
    return matches[0];
}

void object_list::add_object(const std::string &uid, std::shared_ptr<base_object> object) {
    remove(uid);
    objects[uid] = object;

    for (size_t i = 0; i < indices.size(); ++i) {
        indices[i]->add(object);
    }
}

void object_list::remove(const std::string &uuid) { 
    auto itr = objects.find(uuid);

    if (itr != objects.end()) {
        for (size_t i = 0; i < indices.size(); ++i) {
            indices[i]->remove(itr->second);
        }
        objects.erase(itr);
    }
}

object_list &object_list::operator+=(const object_list &other) {
  for (auto &&object : other.objects) {
      add_object(object.first, object.second);
  }
  return *this;
}