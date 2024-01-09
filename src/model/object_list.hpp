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

#ifndef EGILSCIMCLIENT_OBJECT_LIST_H
#define EGILSCIMCLIENT_OBJECT_LIST_H

#include <map>
#include <ostream>
#include <vector>

#include "base_object.hpp"

using object_map_t = std::map<std::string, std::shared_ptr<base_object>>;

/// An index for objects in an object_list
/** The index will let us quickly lookup objects for which a given
 *  attribute has a given value.
 * 
 *  The index assumes the objects don't change while they are indexed.
 */
class object_index {
public:
    object_index(const std::string &attr) : attribute(attr) {}

    const std::string &get_attribute() const { return attribute; }

    // Finds the objects for which attribute == value
    std::vector<std::shared_ptr<base_object>> lookup(const std::string &value);

    // Adds an object to the index
    void add(std::shared_ptr<base_object> object);

    // Removes an object from the index
    void remove(std::shared_ptr<base_object> object);

private:
    // The attribute we're indexing over
    std::string attribute;

    // A map from values to the objects which have that value in the
    // given attribute.
    std::map<std::string, std::vector<std::shared_ptr<base_object>>> idx;
};

class object_list {
    object_map_t objects{};

    std::vector<std::shared_ptr<object_index>> indices;

    std::shared_ptr<object_index> find_index(const std::string& attr) {
        for (size_t i = 0; i < indices.size(); ++i) {
            if (indices[i]->get_attribute() == attr) {
                return indices[i];
            }
        }
        return nullptr;
    }

public:
    object_list() = default;

    object_list(const object_list &other) {
        objects = other.objects;
        indices = other.indices;
    }

    void clear() {
        objects.clear();
        indices.clear();
    }

    // Finds objects that have a given attribute set to a given value.
    std::vector<std::shared_ptr<base_object>> get_objects_for_attribute(const std::string &attribute, const std::string &value);

    // Finds an object that has a given attribute set to a given value.
    // Note that this is supposed to be used when the attribute can be used as a primary key,
    // so it uniquely identifies an object. If there are multiple objects with the attribute
    // set to the value, a random one will be returned.
    // If there is no match, nullptr is returned.
    std::shared_ptr<base_object> get_object_for_attribute(const std::string &attribute, const std::string &id);

    std::shared_ptr<base_object> get_object(const std::string &uid) const {
        auto record = objects.find(uid);
        if (record != objects.end()) {
            return record->second;
        }
        return nullptr;
    }

    void add_object(const std::string &uid, std::shared_ptr<base_object> object);

    void remove(const std::string& uuid);

    object_list &operator+=(const object_list &other);

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

#endif // EGILSCIMCLIENT_OBJECT_LIST_H
