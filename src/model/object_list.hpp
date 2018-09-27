/**
 * Copyright © 2017-2018  Max Wällstedt <>
 *
 * This file is part of SimpleSCIM.
 *
 * SimpleSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with SimpleSCIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef SIMPLESCIM_USER_LIST_H
#define SIMPLESCIM_USER_LIST_H

#include <map>
#include <ostream>

#include "base_object.hpp"

class ScimActions;

typedef std::map<std::string, std::shared_ptr<base_object>> object_map_t;

class object_list {
	object_map_t objects{};
public:
	friend class cache_file;

	object_list() = default;

	object_list(const object_list &other) {
		objects = other.objects;
	}

	object_list(object_list &&other) noexcept {
		objects = std::move(other.objects);
	}

	int process_changes(const object_list &cache, const ScimActions &actions, const std::string &type) const;

	void clear() {
		objects.clear();
	}

	std::shared_ptr<base_object> get_object_for_attribute(const std::string &a, const std::string &id) {
		for (const auto &object : objects) {
			const string_vector &athing = object.second->get_values(a);
			if (!athing.empty() && athing.at(0) == id)
				return object.second;
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

//	void add_object(const std::string &uid, std::shared_ptr<base_object> &&object) {
//		auto record = objects.find(uid);
//		if (record != objects.end()) {
//			objects.erase(uid);
//		}
//		objects.emplace(std::make_pair(uid, std::move(object)));
//	}
//
//	void add_object(const std::string &uid, const std::shared_ptr<base_object> &object) {
//		auto record = objects.find(uid);
//		if (record != objects.end()) {
//			objects.erase(uid);
//		}
//		objects.emplace(std::make_pair(uid, object));
//	}

	void add_object(const std::string &uid, base_object &&object) {
		auto record = objects.find(uid);
		if (record != objects.end()) {
			objects.erase(uid);
		}
		objects.emplace(std::make_pair(uid, std::make_shared<base_object>(std::move(object))));
	}

	object_list &operator+=(const object_list &other) {
		for (auto &&object : other.objects) {
			objects.emplace(object.first, object.second);
		}
		return *this;
	}

	object_list &operator+=(object_list &&other) {
		for (auto &&object : other.objects) {
			objects.emplace(object.first, std::move(object.second));
		}
		return *this;
	}

	object_list &operator=(const object_list &other) = default;

	object_list &operator=(object_list &&other) noexcept {
		objects = std::move(other.objects);
		return *this;
	}

	size_t size() const {
		return objects.size();
	}

	bool empty() const {
		return objects.empty();
	}

	object_map_t::const_iterator begin() {
		return objects.begin();
	}

	object_map_t::const_iterator end() {
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
