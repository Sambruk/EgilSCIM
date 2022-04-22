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

#include "generated_load.hpp"
#include "generated_group_load.hpp"
#include "config_file.hpp"
#include "data_server.hpp"

namespace {

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
        } catch (std::out_of_range &) {
            std::cerr << "Failed to create relational object: " << type << " some relation is missing it's GUID"
                      << std::endl;
        }
    }
    return uuid;
}

/**
 * Generate new Objects of this type
 *
 * @param type the type to generate
 * @return the list
 */
std::shared_ptr<object_list> get_generated_activity(const std::string &type,
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

        auto p1 = string_to_pair(id_cred[0]);
        auto p2 = string_to_pair(id_cred[1]);
        
        // Check that we have the attributes for creating the UUID for this activity
        bool bad = false;
        for (size_t i = 0; i < id_cred.size() && !bad; ++i) {
            if (generated_object.get_values(id_cred[i]).empty()) {
                std::cerr << "Failed to create Activity for group " << student_group.second->get_uid() 
                << "\n\tmissing attribute " << id_cred[i] << " (is the group missing its school unit?)" << std::endl;
                bad = true;
            }
        }

        if (bad) {
            continue;
        }

        std::string uuid = store_relation(generated_object, p1, p2);
        generated->add_object(uuid, std::make_shared<base_object>(generated_object));
        load_logger.log(std::string("Generated ") + type + " with UUID " + uuid +
                        " from " + master_type + " with UUID " + student_group.second->get_uid());
    }
    return generated;
}

/**
 * Generate new Objects of this type
 *
 * @param type the type to generate
 * @return the list
 */
std::shared_ptr<object_list> get_generated_employment(const std::string &type,
                                                      indented_logger& load_logger) {
    config_file &conf = config_file::instance();
    data_server &server = data_server::instance();
    std::set<std::string> missing_ids;
    std::set<std::string> objects_with_missing_ids;

    // the relational key, e.g. User.pidSchoolUnit
    string_pair relational_key = conf.get_pair(type + "-generate-key");
    string_pair part_type = conf.get_pair(type + "-generate-remote-part");

    auto master_list = server.get_by_type(relational_key.first);
    auto related_list = server.get_by_type(part_type.first);

    string_pair related_id = conf.get_pair(type + "-remote-relation-id");

    auto ignore_missing_schoolunit = conf.get_bool(type + "-ignore-missing-schoolunit");

    auto generated = std::make_shared<object_list>();
    if (!master_list)
        return generated;

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
                load_logger.log(std::string("Generated ") + type + " with UUID " + id +
                                " from " + relational_key.first + " with UUID " + a_master.second->get_uid() + " and " + part_type.first + " with UUID " + related_object->get_uid());
            } else {
                missing_ids.insert(relational_item);
                objects_with_missing_ids.insert(relational_key.first + " : " + a_master.first);
            }
        }

    }

    if (!missing_ids.empty() && !ignore_missing_schoolunit) {
        std::cerr << "Missing SchoolUnits found while generating Employments:" <<  std::endl;
        for (const auto &missing_id : missing_ids) {
            std::cerr << missing_id << ", ";
        }
        std::cerr << std::endl;
        std::cerr << "Objects with missing SchoolUnits:" << std::endl;
        int object_count = 0;
        for (const auto &object_with_missing_id : objects_with_missing_ids) {
            std::cerr << object_with_missing_id << ", ";
            if (++object_count >= 10) {
                std::cerr << "...";
                break;
            }
        }
        std::cerr << std::endl;
        
    }

    return generated;
}

} // namespace

void load_related(const std::string &type,
                  const std::shared_ptr<object_list> &objects,
                  indented_logger& load_logger);

std::shared_ptr<object_list> get_generated(const std::string &type,
                                           indented_logger& load_logger) {
    std::shared_ptr<object_list> list;
    if (type == "Activity") {
        list = get_generated_activity(type, load_logger);
    }
    else if (type == "Employment") {
        list = get_generated_employment(type, load_logger);
    }
    else if (type == "StudentGroup") {
        list = get_generated_student_group(type, load_logger);
    }
    else {
        std::cerr << type << " can't be generated" << std::endl;
        return std::make_shared<object_list>();
    }

    // Should we allow relations for the generated types?
    if (list && !list->empty())
        load_related(type, list, load_logger);
    return list;
}
