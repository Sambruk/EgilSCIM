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

#ifndef SIMPLESCIM_USER_LIST_H
#define SIMPLESCIM_USER_LIST_H

#include <map>
#include <ostream>

#include "base_object.hpp"

using object_map_t = std::map<std::string, std::shared_ptr<base_object>>;

class object_list {
    object_map_t objects{};
public:
    object_list() = default;

    object_list(const object_list &other) {
        objects = other.objects;
    }

    void clear() {
        objects.clear();
    }

    std::shared_ptr<base_object> get_object_for_attribute(const std::string &attribute, const std::string &id) {
        for (const auto &object : objects) {
            const string_vector &values = object.second->get_values(attribute);
            for (auto && value: values) {
                if (value == id)
                    return object.second;
            }
        }
        return nullptr;
    }

    std::shared_ptr<base_object> get_object(const std::string &uid) const {
        auto record = objects.find(uid);
        if (record != objects.end()) {
            return record->second;
        }
        return nullptr;
    }

    void add_object(const std::string &uid, std::shared_ptr<base_object> object) {
        objects[uid] = object;
    }

    void remove(const std::string& uuid) {
        objects.erase(uuid);
    }

    object_list &operator+=(const object_list &other) {
        for (auto &&object : other.objects) {
            objects.emplace(object.first, object.second);
        }
        return *this;
    }
    object_list &operator=(const object_list &other) = default;

    size_t size() const {
        return objects.size();
    }

    bool empty() const {
        return objects.empty();
    }

    object_map_t::const_iterator begin() const {
        return objects.begin();
    }

    object_map_t::const_iterator end() const {
        return objects.end();
    }

    friend std::ostream &operator<<(std::ostream &os, const object_list &list) {
        for (const auto &item : list.objects) {
            os << *item.second;
        }
        return os;
    }
};

#endif
