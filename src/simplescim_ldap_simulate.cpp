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

#include "simplescim_ldap.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory>

#include "utility/simplescim_error_string.hpp"
//#include "model/value_list.hpp"
#include "model/base_object.hpp"
#include "model/object_list.hpp"

struct attribute {
	const char *name;
	size_t n_values;
	const char *values[10];
};

struct object {
	size_t n_attributes;
	struct attribute attributes[10];
};

static struct object g_users[] = {
		{6, {
				    {"uid", 1, {"test1"}},
				    {"fullName", 1, {"Test1 Test2est31"}},
				    {"givenName", 1, {"Test1"}},
				    {"pidSchoolUnit", 1, {"12345678"}},
				    {"l", 1, {"test1"}},
				    {"email", 3, {
						                 "test1#examples.com",
						                 "test1#email.com",
						                 "spam#hotmail.com"
				                 }}
		    }},
		{6, {
				    {"uid", 1, {"test2"}},
				    {"fullName", 1, {"Test2 Testtest2"}},
				    {"givenName", 1, {"Testname2"}},
				    {"pidSchoolUnit", 1, {"12345678"}},
				    {"l", 1, {"test2"}},
				    {"email", 3, {
						                 "test2#example.com",
						                 "test2#email.com",
						                 "test2#gmail.com"
				                 }}
		    }},
		{6, {
				    {"uid", 1, {"test31"}},
				    {"fullName", 1, {"Test3 Testtest3"}},
				    {"givenName", 1, {"Testname"}},
				    {"pidSchoolUnit", 1, {"12345678"}},
				    {"l", 1, {"test3"}},
				    {"email", 3, {
						                 "test3#example.com",
						                 "test3#email.com",
						                 "test3#pmail.com"
				                 }}
		    }}
};
//
//static struct object g_courses[] = {
//	{4, {
//		{"cid", 1, {"geo"}},
//		{"displayName", 1, {"Geografi"}},
//		{"email", 1, {
//			"geografi#skola.se"
//		}},
//		{"members", 2, {
//			"test1",
//			"test2"
//		}}
//	}},
//	{4, {
//		{"gid", 1, {"Historia"}},
//		{"displayName", 1, {"Historia"}},
//		{"email", 1, {
//			"historia#skolan.se"
//		}},
//		{"members", 3, {
//			"test1",
//			"test2",
//			"test3"
//		}}
//	}},
//};

static std::shared_ptr<std::vector<std::string>> get_values(struct attribute *ap) {
	auto values = std::make_shared<std::vector<std::string>>();

	for (int i = 0; i < ap->n_values; ++i) {
		values->emplace_back(ap->values[i]);
	}

	return values;
}

std::shared_ptr<base_object> get_objects(struct object *up) {
	std::shared_ptr<std::vector<std::string>> values;
	std::shared_ptr<base_object> userp = std::make_shared<base_object>(base_object());

	for (size_t i = 0; i < up->n_attributes; ++i) {
		std::string attribute = up->attributes[i].name;

		values = get_values(&up->attributes[i]);

		if (values == nullptr) {
			userp.reset();
			return nullptr;
		}
		userp->add_attribute(attribute, std::move(*values));
	}
	return userp;
}


std::shared_ptr<object_list> simplescim_ldap_get_objects(object *up, size_t size) {
	std::shared_ptr<object_list> objects = std::make_shared<object_list>();

	for (int i = 0; i < size / sizeof(struct object); ++i) {
		std::shared_ptr<base_object> object = get_objects(&up[i]);
		if (object == nullptr) {
			return nullptr;
		}
		std::string uid = object->get_uid();
		if (uid.empty()) {
			return nullptr;
		}
		objects->add_object(uid, std::move(*object));
	}
	return objects;
}

std::shared_ptr<object_list> ldap_get_users() {
	return simplescim_ldap_get_objects(g_users, sizeof(g_users));
}
