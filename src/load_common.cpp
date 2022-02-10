/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2021 Föreningen Sambruk
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

#include "load_common.hpp"
#include "config_file.hpp"
#include "data_server.hpp"
#include "json_data_file.hpp"
#include "simplescim_ldap.hpp"
#include "load_limiter.hpp"
#include <cassert>

std::shared_ptr<object_list> filter_objects(std::shared_ptr<object_list> objects,
                                            std::shared_ptr<load_limiter> limiter,
                                            indented_logger &load_logger,
                                            const std::string& type) {
    auto included_objects = std::make_shared<object_list>();

    for (auto &iter : *objects) {
        auto object = iter.second;
        auto uid = object->get_uid();
        assert(!uid.empty());
        if (limiter->include(object.get())) {
            included_objects->add_object(uid, object);
            load_logger.log("Found " + type + " " + uid);
        }
    }

    return included_objects;
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

                        ldap_wrapper& ldap = *server.get_ldap_wrapper();

                        if (!ldap.valid()) {
                            std::cerr << "can't connect to LDAP" << std::endl;
                            throw std::runtime_error("failed to connect to LDAP");
                        }

                        if (ldap.search(relation.type, load_logger, filter)) {
                            auto limiter = get_limiter(relation.type);
                            auto response = ldap_to_object_list(ldap, relation.type, limiter, load_logger);
                            if (response->size() == 1) {
                                remote = response->begin()->second;
                                server.add(relation.type, remote);
                            }
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
                    }
                }
            }
        }
    }
}
