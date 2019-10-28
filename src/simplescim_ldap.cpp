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

#include "simplescim_ldap.hpp"

#include <set>
#include "utility/simplescim_error_string.hpp"
#include "model/base_object.hpp"
#include "model/object_list.hpp"
#include "config_file.hpp"
#include "json_data_file.hpp"
#include "utility/utils.hpp"
#include "data_server.hpp"
#include "ldap_wrapper.hpp"
#include "load_limiter.hpp"


std::string
store_relation(base_object &generated_object,
               const std::pair<std::string, std::string> &part_type,
               const std::pair<std::string, std::string> &master_id);

/**
 * Construct the object list from the LDAP response.
 * On success, a pointer to the constructed list is
 * returned. On error, nullptr is returned and
 * simplescim_error_string is set to an appropriate
 * error message.
 */
std::shared_ptr<object_list> ldap_to_object_list(ldap_wrapper& ldap,
                                                 const std::string& type,
                                                 std::shared_ptr<load_limiter> limiter,
                                                 indented_logger& load_logger) {
  std::shared_ptr<object_list> objects;
  std::string uid;

  objects = std::make_shared<object_list>();
  std::shared_ptr<base_object> obj = ldap.first_object();
  {
      indented_logger::indenter indenter(load_logger);
      while (obj != nullptr) {
          uid = obj->get_uid();

          if (!uid.empty() && limiter->include(obj.get())) {
              objects->add_object(uid, obj);

              load_logger.log("Found " + type + " " + uid);
          }

          obj = ldap.next_object();
      }
  }
  load_related(type, objects, load_logger);
  return objects;
}  

/**
 * Create a new UUID for a relation, based on two UUIDs.
 *
 * It's important that this function is deterministic and always
 * generates the same new UUID for a given pair of UUIDs.
 *
 * This needs to be maintained through new versions of EGIL.
 */
std::string create_relational_id(const string_pair &index_fields) {
  return uuid_util::instance().generate(index_fields.first, index_fields.second);
}

/**
 * Generate new Objects of this type
 *
 * @param type the type to generate
 * @return the list
 */
std::shared_ptr<object_list> ldap_get_generated_activity(const std::string &type,
                                                         indented_logger& load_logger) {
    config_file &conf = config_file::instance();

    auto generated = std::make_shared<object_list>();

    // get by type only, the type must be loaded already so no query needed

    std::string master_type;
    if (conf.has(type + "-generate-key")) {
        string_pair generate_key = conf.get_pair(type + "-generate-key");
        master_type = generate_key.first;

        std::cout << type + "-generate-key is obsolete, please specify " + type + "-generate-type instead" << std::endl;
    }
    if (conf.has(type + "-generate-type")) {
        master_type = conf.get(type + "-generate-type");
    }

    if (master_type == "") {
        std::cerr << "Missing parameter " + type + "-generate-type" << std::endl;
        return nullptr;
    }
    
    std::string remote_relation = conf.get(type + "-remote-relation-id");
    string_pair related_type = conf.get_pair(type + "-generate-remote-part");
    string_vector scim_vars = conf.get_vector(type + "-scim-variables");
    string_pair local_relation = conf.get_pair(type + "-generate-local-part");
    std::string uuid_attribute = conf.get(type + "-unique-identifier");
    string_vector id_cred = conf.get_vector(type + "-GUID-generation-ids");

    if (id_cred.size() != 2) {
        std::cerr << type << "-GUID-generation-ids must be 2 relations like StudentGroup.GUID SchoolUnit.GUID"
                  << std::endl;
        return nullptr;
    }

    data_server &server = data_server::instance();
    auto student_groups = server.get_by_type(master_type);
    auto employments = server.get_by_type(related_type.first);

    for (const auto &student_group : *student_groups) {
        base_object generated_object(type);
        generated_object.add_attribute(pair_to_string(local_relation),
                                       student_group.second->get_values(local_relation.second));

        string_vector members = student_group.second->get_values(remote_relation);
        for (auto &&member: members) {
            auto employment = data_server::instance().find_object_by_attribute(related_type.first, remote_relation,
                                                                               member);
            if (employment) {
                generated_object.append_values(pair_to_string(related_type), {employment->get_uid()});
            }
        }

        for (auto &&scim_var: scim_vars) {

            if (scim_var == uuid_attribute)
                continue;

            auto var_pair = string_to_pair(scim_var);
            if (var_pair.first == master_type) {
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

        auto p1 = string_to_pair(id_cred.at(0));
        auto p2 = string_to_pair(id_cred.at(1));
        std::string uuid = store_relation(generated_object, p1, p2);
        generated->add_object(uuid, std::make_shared<base_object>(generated_object));

    }
    return generated;
}

/**
 * Generate new Objects of this type
 *
 * @param type the type to generate
 * @return the list
 */
std::shared_ptr<object_list> ldap_get_generated_employment(const std::string &type,
                                                           indented_logger& load_logger) {
    config_file &conf = config_file::instance();
    data_server &server = data_server::instance();
    std::set<std::string> missing_ids;

    // the relational key, e.g. User.pidSchoolUnit
    string_pair relational_key = conf.get_pair(type + "-generate-key");
    string_pair part_type = conf.get_pair(type + "-generate-remote-part");

    auto master_list = server.get_by_type(relational_key.first);
    auto related_list = server.get_by_type(part_type.first);

    string_pair related_id = conf.get_pair(type + "-remote-relation-id");

    auto generated = std::make_shared<object_list>();
    if (!master_list)
        return generated;

    ldap_wrapper ldap;

    for (const auto &a_master: *master_list) {
        string_vector relational_items = a_master.second->get_values(relational_key.second);
        if (relational_items.empty()) {

            std::cerr << "Creating Employment: didn't find values for " << relational_key.second
                      << " for " << relational_key.first << " " << a_master.first
                      << std::endl;
        }

        // for each entry create a new relational object and
        // decorate it with some more attributes from the config.
        for (const auto &relational_item : relational_items) {
            base_object generated_object(type);
            generated_object.add_attribute(pair_to_string(relational_key), {relational_item});

            // don't ask data_server for missing id's if we already know it's missing
            if (missing_ids.find(relational_item) != missing_ids.end())
                continue;
            std::shared_ptr<base_object> related_object;
            related_object = data_server::instance().find_object_by_attribute(part_type.first, related_id.second,
                                                                              relational_item);

            if (related_object) {
                std::pair master_id = conf.get_pair(type + "-generate-local-part");

                string_vector scim_vars = conf.get_vector_sorted_unique(type + "-scim-variables");
                scim_vars.emplace_back(conf.get(type + "-hidden-attributes", true));
                for (const auto &var : scim_vars) {
                    auto var_pair = string_to_pair(var);
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
                std::string id = store_relation(generated_object, part_type, master_id);
                generated->add_object(id, std::make_shared<base_object>(generated_object));
            } else {
                missing_ids.insert(relational_item);
            }
        }

    }

    if (!missing_ids.empty()) {
        std::cerr << "Missing SchoolUnits found:" << std::endl;
        for (const auto &missing_id : missing_ids) {
            std::cerr << missing_id << ", ";
        }
        std::cerr << std::endl;
    }

    return generated;
}

std::string store_relation(base_object &generated_object,
                           const std::pair<std::string, std::string> &part_type,
                           const std::pair<std::string, std::string> &master_id) {
    config_file &conf = config_file::instance();
    std::string type = generated_object.getSS12000type();
    std::string uuid;

    if (!generated_object.get_values(pair_to_string(part_type)).empty()) {

        try {
            auto relational_id_pair = std::make_pair(
                    generated_object.get_values(pair_to_string(part_type)).at(0),
                    generated_object.get_values(pair_to_string(master_id)).at(0));

            uuid = create_relational_id(relational_id_pair);
            if (!uuid.empty()) {
                generated_object.add_attribute(conf.get(type + "-unique-identifier"), {uuid});
            } else {
                std::cerr << "failed to generate relation, can't create its ID" << std::endl;
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
void load_related(const std::string &type,
                  const std::shared_ptr<object_list> &objects,
                  indented_logger& load_logger) {
    config_file &conf = config_file::instance();
    data_server &server = data_server::instance();

    relations_vector relations =
            json_data_file::json_to_ldap_remote_relations(
                conf.get(type + "-remote-relations", true), type);
    if (relations.empty())
        return;

    string_vector scim_vars = conf.get_vector_sorted_unique(type + "-scim-variables");

    ldap_wrapper ldap;

    indented_logger::indenter indenter(load_logger);
    
    for (auto &&main_object: *objects) {
        for (auto &&relation: relations) {
            load_logger.log("Finding " + relation.type + " objects for " + type + " (" + main_object.second->get_uid() + ")");
            indented_logger::indenter indenter(load_logger);
            if (relation.method == "object") {
                auto relation_source = data_server::instance().get_by_type(relation.type);
                if (relation_source) {
                    string_vector values = main_object.second->get_values(relation.local_attribute);
                    for (size_t i = 0; i < values.size(); ++i) {
                        auto remote_object = server.find_object_by_attribute(relation.type,
                                                                             relation.remote_attribute, values[i]);
                        if (remote_object) {
                            for (auto &&var : scim_vars) {
                                auto p = string_to_pair(var);
                                if (p.first == relation.type) {
                                    string_vector v = remote_object->get_values(p.second);
                                    main_object.second->append_values(var, v);
                                }
                            }

                            auto relations_scim_vars = conf.get_vector(relation.type + "-scim-variables");
                            for (auto &&var : relations_scim_vars) {
                                auto p = string_to_pair(var);
                                if (p.first == type) {
                                    auto id = main_object.second->get_values(p.second);
                                    remote_object->append_values(type + "." + p.second, id, true);
                                }
                            }
                            
                        }
                    }
                }
            } else if (relation.method == "ldap") {
                string_vector values = main_object.second->get_values(relation.local_attribute);
                for (auto &&value : values) {
                    // first check it if's cached already

                    std::shared_ptr<base_object> remote = server.find_object_by_attribute(relation.type,
                                                                                          relation.remote_attribute,
                                                                                          value);
                    if (!remote) {

                        auto filter = relation.get_ldap_filter(value);

                        if (ldap.search(relation.type, load_logger, filter)) {
                            auto limiter = get_limiter(relation.type);
                            auto response = ldap_to_object_list(ldap, relation.type, limiter, load_logger);
                            if (response->size() == 1)
                                remote = response->begin()->second;
                            else if (response->size() > 1) {
                                std::cerr << "Ambiguous (multiple) results for relation from " << type
                                          << " to " << relation.type << " when " << type << "." << relation.local_attribute
                                          << " = " << value << std::endl;
                            }
                        }
                    }
                    if (remote) {
                        // from the newly loaded related object, grab some info from it
                        // e.g. a StudentGroup loads Students, grab their names and GUIDs
                        // it is whatever is in the json-config with the related objects TYPE
                        for (auto &&var : scim_vars) {
                            auto p = string_to_pair(var);
                            if (p.first == relation.type) {
                                string_vector v = remote->get_values(p.second);
                                main_object.second->append_values(var, v);
                            }
                        }
                        // Sometimes the related object need some info about this object, hand it down
                        auto relations_scim_vars = conf.get_vector(relation.type + "-scim-variables");
                        for (auto &&var : relations_scim_vars) {
                            auto p = string_to_pair(var);
                            if (p.first == type) {
                                auto id = main_object.second->get_values(p.second);
                                remote->append_values(type + "." + p.second, id, true);
                            }
                        }
                        // add the new entity to the server
                        server.cache_relation(relation.type + relation.remote_attribute + value, remote);
                        server.add(relation.type, remote);
                    }
                }

            }
        }
    }
}

std::shared_ptr<object_list> ldap_get_generated(const std::string &type,
                                                indented_logger& load_logger) {
    std::shared_ptr<object_list> list;
    if (type == "Activity")
        list = ldap_get_generated_activity(type, load_logger);
    else if (type == "Employment")
        list = ldap_get_generated_employment(type, load_logger);
    else {
        std::cerr << type << " can't be generated" << std::endl;
        return std::make_shared<object_list>();
    }
    if (list && !list->empty())
        load_related(type, list, load_logger);
    return list;
}

/**
 * Reads user data from LDAP into a user list object
 * according to the configuration file and returns a
 * pointer it. On error, nullptr is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
std::shared_ptr<object_list> ldap_get(ldap_wrapper &ldap,
                                      const std::string &type,
                                      indented_logger& load_logger) {

    load_logger.log(std::string("Loading entries for type ") + type);
    indented_logger::indenter indenter(load_logger);
    
    config_file &conf = config_file::instance();

    std::shared_ptr<object_list> objects;
    if (conf.get_bool(type + "-is-generated")) {
        objects = ldap_get_generated(type, load_logger);
    } else {
        if (ldap.search(type, load_logger)) {
            auto limiter = get_limiter(type);            
            objects = ldap_to_object_list(ldap, type, limiter, load_logger);
        }
    }

    return objects;
}
