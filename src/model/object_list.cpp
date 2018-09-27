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


#include "object_list.hpp"
#include <iostream>
#include "../scim.hpp"

#include "../utility/simplescim_error_string.hpp"

/**
 * digg out the calculated entries not in ldap
 */


/**
 * Compares 'this' to 'cache' and performs 'copy_user_func'
 * on users in both 'this' and cache if they are equal,
 * 'create_user_func' on users in 'this' but not in
 * 'cache', performs 'update_user_func' on users in both
 * 'this' and 'cache' if the user has been updated and
 * performs 'delete_user_func' on users in 'cache' but not
 * in 'this'.
 */
int object_list::process_changes(const object_list &cache, const ScimActions &actions, const std::string &type) const {
	int err;
	struct statistics {
		size_t n_copy = 0, n_copy_fail = 0;
		size_t n_create = 0, n_create_fail = 0;
		size_t n_update = 0, n_update_fail = 0;
		size_t n_delete = 0, n_delete_fail = 0;
	} stats{};

	for (auto &&iter : this->objects) {

		const std::string &uid = iter.first;

		auto object = iter.second;
		auto cached_object = cache.get_object(uid);

		if (cached_object == nullptr) {
			++stats.n_create;
			auto create_functor = ScimActions::create_func(*object);
			err = create_functor(actions);

			if (err == -1) {
				++stats.n_create_fail;
				std::cerr << simplescim_error_string_get() << std::endl;
			}
		} else {
			/* User exists in 'cache' */
			if (*object == *cached_object) {

				// User is the same, copy it
				++stats.n_copy;
				auto copy_functor = ScimActions::copy_func(*cached_object);
				if (copy_functor(actions) == -1) {
					++stats.n_copy_fail;
					std::cerr << simplescim_error_string_get() << std::endl;
				}
			} else {
				/** User is different, update it */
//				std::cout << "Sending changes for:" << std::endl;
//				std::cout << *object << std::endl;
//				std::cout << *cached_object << std::endl;
//				std::cout << "------------------------------" << std::endl;
				++stats.n_update;
				ScimActions::update_func update_f(*object, *cached_object);

				if (update_f(actions) == -1) {
					++stats.n_update_fail;
					std::cerr << simplescim_error_string_get() << std::endl;
				}
			}
		}
	}

	/** For every user in 'cache' of the given type */
	for (auto &&item : cache.objects) {
		std::shared_ptr<base_object> object = item.second;
		if (object->getSS12000type() == type) {
			const std::string &uid = item.first;
			// Get thing from 'this'
			auto tmp = this->get_object(uid);

			if (tmp == nullptr) {
				// User doesn't exist in 'this', delete it
				++stats.n_delete;
				auto delete_f = ScimActions::delete_func(*object);
				err = delete_f(actions);

				if (err == -1) {
					++stats.n_delete_fail;
					fprintf(stderr, "%s\n", simplescim_error_string_get());
				}
			}
		}
	}

	printf("Status:   Success   Failure     Total  of type: %s\n", type.c_str());
	printf("Copy:   %9lu %9lu %9lu\n", stats.n_copy - stats.n_copy_fail, stats.n_copy_fail, stats.n_copy);
	printf("Create: %9lu %9lu %9lu\n", stats.n_create - stats.n_create_fail, stats.n_create_fail, stats.n_create);
	printf("Update: %9lu %9lu %9lu\n", stats.n_update - stats.n_update_fail, stats.n_update_fail, stats.n_update);
	printf("Delete: %9lu %9lu %9lu\n", stats.n_delete - stats.n_delete_fail, stats.n_delete_fail, stats.n_delete);

	return 0;
}
