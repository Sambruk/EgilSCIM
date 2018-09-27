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
#ifndef SIMPLESCIM_SCIM_H
#define SIMPLESCIM_SCIM_H

#include <map>
#include <string>
#include "config_file.hpp"
#include "data_server.hpp"
#include <memory>

class base_object;

class object_list;

struct variables {
	std::map<std::string, std::string> variable_entries;

	variables();

	bool valid() const {
		for (const auto &entry : variable_entries) {
			if (entry.second.empty())
				return false;
		}
		return true;
	}

	std::string get(const std::string &key) const {
		return variable_entries.find(key)->second;
	}
};

class ScimActions {
	std::shared_ptr<object_list> scim_new_cache;
	variables vars = variables();
	const config_file &conf = config_file::instance();

	void simplescim_scim_clear() const;

	int simplescim_scim_init() const;

public:
	ScimActions() {
		scim_new_cache = std::make_unique<object_list>();
	}

	int perform(const data_server &current, const object_list &cached) const;

	class copy_func {
		const base_object &cached;
	public:
		explicit copy_func(const base_object &o) : cached(o) {}

		int operator()(const ScimActions &);
	};

	class create_func {
		const base_object &create;
	public:
		explicit create_func(const base_object &c) : create(c) {}

		int operator()(const ScimActions &);
	};

	class update_func {
		const base_object &object;
//		const base_object &cached_object;
	public:
		update_func(const base_object &o, const base_object &co) : object(o)
//		, cached_object(co)
		{}

		int operator()(const ScimActions &);
	};

	class delete_func {
		const base_object &object;
	public:
		explicit delete_func(const base_object &o) : object(o) {}

		int operator()(const ScimActions &);
	};
};


#endif
