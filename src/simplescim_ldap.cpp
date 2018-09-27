#include <utility>

#include <utility>

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

#include <stdlib.h>
#include <string.h>
#include <set>
#include <uuid/uuid.h>

#include "utility/simplescim_error_string.hpp"
//#include "moved_out/value_list.hpp"
#include "model/base_object.hpp"
#include "model/object_list.hpp"
#include "config_file.hpp"
#include "simplescim_ldap_attrs_parser.hpp"
#include "json_data_file.hpp"
#include "utility/utils.hpp"
#include "local_id_store.hpp"
#include "data_server.hpp"
#include "ldap_wrapper.hpp"


void load_related(const std::string &type, const std::shared_ptr<object_list> &objects);

std::string
store_relation(local_id_store &persister, base_object &generated_object,
               const std::pair<std::string, std::string> &part_type,
               const std::pair<std::string, std::string> &master_id);

std::shared_ptr<object_list> get_object_list_by_type(const std::string &type, const pair_map &queries) {
	data_server &server = data_server::instance();

	std::shared_ptr<object_list> list = server.get_by_type(type);
	if (!queries.empty()) {
		auto q = queries.find(type);
		ldap_wrapper ldap;
		if (ldap.search(type, q->second))
			list = ldap.ldap_to_user_list();
		server.add(type, list);
	}
	return list;
}

/**
 * Generate new Objects of this type
 *
 * @param type the type to generate
 * @return the list
 */
std::shared_ptr<object_list> ldap_get_generated_activity(const std::string &type) {
	config_file &conf = config_file::instance();
	local_id_store persister;
	if (!persister.is_open())
		return {};

	auto generated = std::make_shared<object_list>();

	// get by type only, the type must be loaded already so no query needed
	std::string remote_relation = conf.get(type + "-remote-relation-id");
	string_pair master_type = conf.get_pair(type + "-generate-key");
	string_pair related_type = conf.get_pair(type + "-generate-remote-part");
	string_vector scim_vars = conf.get_vector(type + "-scim-variables");
	string_pair local_relation = conf.get_pair(type + "-local-relation-id");
	std::string uuid_attribute = conf.get(type + "-unique-identifier");
	string_vector id_cred = conf.get_vector(type + "-GUID-generation-ids");

	if (id_cred.size() != 2) {
		std::cerr << type << "-GUID-generation-ids must be 2 relations like StudentGroup.GUID SchoolUnit.GUID" << std::endl;
		return nullptr;
	}
	auto student_groups = get_object_list_by_type(master_type.first, pair_map());
	auto employments = get_object_list_by_type(related_type.first, pair_map());

	for (const auto &student_group : *student_groups) {
		base_object generated_object(type);
		generated_object.add_attribute(pair_to_string(local_relation), student_group.second->get_values(local_relation.second));

		string_vector members = student_group.second->get_values(remote_relation);
		for (auto &&member: members) {
			auto employment = employments->get_object_for_attribute(remote_relation, member);
			if (employment) {
				generated_object.append_values(pair_to_string(related_type), {employment->get_uid()});
			}
		}

		for (auto &&scim_var: scim_vars) {

			if (scim_var == uuid_attribute)
				continue;

			string_pair var_pair = string_to_pair(scim_var);
			if (var_pair.first == master_type.first) {
				// those are from the object it self, found by var_pair.second
				string_vector values = student_group.second->get_values(var_pair.second);
				if (!values.empty())
					generated_object.add_attribute(scim_var, values);
			} else {
				string_vector values = student_group.second->get_values(scim_var);
				if (!values.empty())
					generated_object.add_attribute(scim_var, values);
			}
		}

			std::pair<std::string, std::string> p1 = string_to_pair(id_cred.at(0));
			std::pair<std::string, std::string> p2 = string_to_pair(id_cred.at(1));
			std::string uuid = store_relation(persister, generated_object, p1, p2);
			generated->add_object(uuid, std::move(generated_object));

	}

	return generated;
}


/**
 * Generate new Objects of this type
 *
 * @param type the type to generate
 * @return the list
 */
std::shared_ptr<object_list> ldap_get_generated_employment(const std::string &type) {
	config_file &conf = config_file::instance();
	local_id_store persister;
	if (!persister.is_open())
		return {};
	std::set<std::string> missing_ids;
	std::cout << "Generating " << type;
	// the relational key, e.g. User.pidSchoolUnit
	string_pair relational_key = conf.get_pair(type + "-generate-key");
	string_pair part_type = conf.get_pair(type + "-generate-remote-part");

	pair_map queries = json_data_file::json_to_ldap_query(conf.get(type + "-ldap-filter"));

	auto master_list = data_server::instance().get_static_by_type(relational_key.first);
	if (master_list->empty())
		master_list = get_object_list_by_type(relational_key.first, queries);
	auto related_list = data_server::instance().get_static_by_type(part_type.first);
	if (related_list->empty())
		related_list = get_object_list_by_type(part_type.first, queries);

	string_pair related_id = conf.get_pair(type + "-remote-relation-id");

	auto generated = std::make_shared<object_list>();

	ldap_wrapper ldap;

	for (const auto &a_master: *master_list) {

		string_vector relational_items = a_master.second->get_values(relational_key.second);

		// for each entry create a new relational object and
		// decorate it with some more attributes from the config.
		for (const auto &relational_item : relational_items) {
			base_object generated_object(type);
			generated_object.add_attribute(pair_to_string(relational_key), {relational_item});

			std::shared_ptr<base_object> related_object;
			related_object = related_list->get_object_for_attribute(related_id.second, relational_item);

			if (related_object) {
				std::pair master_id = conf.get_pair(type + "-local-relation-id");

				string_vector scim_vars = conf.get_vector_sorted_unique(type + "-scim-variables");
				scim_vars.emplace_back(conf.get(type + "-hidden-attributes", true));
				for (const auto &var : scim_vars) {
					string_pair var_pair = string_to_pair(var);

					if (var_pair.first == relational_key.first && var_pair.second != relational_key.second) {
						// get info from the main object, except the relational attribute
						string_vector attributes = a_master.second->get_values(var_pair.second);
						generated_object.add_attribute(var, attributes);
					} else if (var_pair.first == part_type.first) {
						// grab this attribute's values from the related object
						if (related_object != nullptr) {
							string_vector attributes = related_object->get_values(var_pair.second);
							generated_object.add_attribute(var, attributes);
						}
					}
				}
				// create an id for the relation
				std::string id = store_relation(persister, generated_object, part_type, master_id);
				generated->add_object(id, std::move(generated_object));
			} else {
				missing_ids.insert(relational_item);
			}
		}
	}

	std::cout << " - done!" << std::endl;
	if (!missing_ids.empty()) {
		std::cerr << "Missing SchoolUnits found:" << std::endl;
		for (
			const auto &missing_id : missing_ids) {
			std::cerr << missing_id << std::endl;
		}
	}

	return generated;
}

std::string store_relation(local_id_store &persister, base_object &generated_object,
                           const std::pair<std::string, std::string> &part_type,
                           const std::pair<std::string, std::string> &master_id) {
	config_file &conf = config_file::instance();
	std::string type = generated_object.getSS12000type();
	std::string uuid;
	if (!persister.is_open())
		return {};
	if (!generated_object.get_values(pair_to_string(part_type)).empty()) {

		try {
			auto relational_id_pair = std::make_pair(
					generated_object.get_values(pair_to_string(part_type)).at(0),
					generated_object.get_values(pair_to_string(master_id)).at(0));

			auto uuid_str = persister.get_relational_id(relational_id_pair);
			if (!uuid_str) {
				uuid_str = persister.create_relational_id(relational_id_pair);
			}
			if (uuid_str) {
				uuid = *uuid_str;
				generated_object.add_attribute(conf.get(type + "-unique-identifier"), {uuid});
			} else {
				std::cerr << "failed to generate relation, can't create and save it's ID" << std::endl;
			}
		} catch (std::out_of_range &oor) {
			std::cerr << "Failed to create relational object: " << type << " some relation is missing it's GUID"
			          << std::endl;
		}
	}
	return uuid;
}



/**
 * for type that have "meta data", i.e. a reference to another type, fetch the corresponding
 * data from data_server or ldap and fill the missing information.
 * @param type
 * @param objects
*/
void load_related(const std::string &type, const std::shared_ptr<object_list> &objects) {
	config_file &conf = config_file::instance();
	data_server &server = data_server::instance();
	string_vector scim_vars = conf.get_vector_sorted_unique(type + "-scim-variables");
	relations_vector relations =
			json_data_file::json_to_ldap_remote_relations(
					conf.get(type + "-remote-relations", true));
	if (relations.empty())
		return;
	ldap_wrapper ldap;

	for (auto &&main_object: *objects) {
		for (auto &&relation: relations) {
			if (relation.method == "object") {
				auto relation_source = data_server::instance().get_by_type(relation.type);
				if (relation_source) {
					string_vector values = main_object.second->get_values(relation.local_attibute);
					if (values.size() == 1) {
						auto remote_object = relation_source->get_object_for_attribute(relation.remote_attribute,
						                                                               values.at(0));
						for (auto &&var : scim_vars) {
							string_pair p = string_to_pair(var);
							if (p.first == relation.type) {
								string_vector v = remote_object->get_values(p.second);
								main_object.second->add_attribute(var, v);
							}
						}
					}
				}
			} else if (relation.method == "ldap") {
				string_vector values = main_object.second->get_values(relation.local_attibute);
				for (auto &&value : values) {
					std::shared_ptr<base_object> remote = server.find_object_by_attribute(relation.type,
					                                                                      relation.remote_attribute,
					                                                                      value);
					if (!remote) {
						if (ldap.search(relation.type, {value, "(objectClass=*)"})) {
							auto response = ldap.ldap_to_user_list();
							if (response->size() != 1)
								std::cout << relation.type + " not found with: " << value << std::endl;
							else {
								remote = response->begin()->second;
								remote->add_attribute(relation.remote_attribute, {value});
							}
						}
					}
					if (remote) {

						for (auto &&var : scim_vars) {
							string_pair p = string_to_pair(var);
							if (p.first == relation.type) {
								string_vector v = remote->get_values(p.second);
								main_object.second->append_values(var, v);
							}
						}
						auto id = main_object.second->get_values("GUID");
						remote->append_values(type + ".GUID", id, true);
						// add the new entity to the server
						server.add(relation.type, std::move(*remote));
					}
				}

			}
		}
	}
}

std::shared_ptr<object_list> ldap_get_generated(const std::string &type) {
	if (type == "Activity")
		return ldap_get_generated_activity(type);
	else if (type == "Employment")
		return ldap_get_generated_employment(type);
	else {
		std::cerr << type << " can't be generated" << std::endl;
		return std::make_shared<object_list>();
	}
}

/**
 * Reads user data from LDAP into a user list object
 * according to the configuration file and returns a
 * pointer it. On error, nullptr is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
std::shared_ptr<object_list> ldap_get(ldap_wrapper &ldap, const std::string &type) {
	config_file &conf = config_file::instance();

	std::shared_ptr<object_list> objects;
	if (conf.get_bool(type + "-is-generated")) {
		objects = ldap_get_generated(type);
		if (objects)
			load_related(type, objects);
	} else {
		if (ldap.search(type)) {
			/** Create user list. */
			objects = ldap.ldap_to_user_list();
			if (objects)
				load_related(type, objects);
		}
	}

	return objects;
}
