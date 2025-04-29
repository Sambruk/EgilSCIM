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
#include "config.hpp"
#include "data_server.hpp"
#include "json_data_file.hpp"
#include "simplescim_ldap.hpp"
#include "load_limiter.hpp"
#include "readable_id.hpp"
#include <cassert>
#include <regex>

void transform_objects(std::shared_ptr<object_list> objects, std::shared_ptr<transformer> transform) {
    for (auto &iter : *objects) {
        auto object = iter.second;
        transform->apply(object.get());
    }
}

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
            load_logger.log("Found " + type + " " + readable_id(object.get(), type));
        } else if (config::load_log_include_skipped()) {
            load_logger.log("Skipping " + type + " " + readable_id(object.get(), type) + " due to load limiting");
        }
    }

    return included_objects;
}

void establish_relation(std::shared_ptr<base_object> main_object,
                        std::shared_ptr<base_object> remote,
                        const std::string& main_type,
                        const std::string& remote_type,
                        indented_logger& load_logger) {
    
    config_file &conf = config_file::instance();

    if (conf.get_bool("load-log-include-relations")) {
        load_logger.log("Relation (" + main_type + " -> " + remote_type + "): " + readable_id(main_object.get(), main_type) + " -> " + readable_id(remote.get(), remote_type));
    }

    string_vector main_scim_vars = conf.get_vector_sorted_unique(main_type + "-scim-variables");
    string_vector remote_scim_vars = conf.get_vector_sorted_unique(remote_type + "-scim-variables");

    // from the newly loaded related object, grab some info from it
    // e.g. a StudentGroup loads Students, grab their names and GUIDs
    // it is whatever is in the json-config with the related objects TYPE
    for (auto &&var : main_scim_vars) {
        auto p = string_to_pair(var);
        if (p.first == remote_type) {
            string_vector v = remote->get_values(p.second);
            main_object->append_values(var, v);
        }
    }
    // Sometimes the related object need some info about this object, hand it down
    for (auto &&var : remote_scim_vars) {
        auto p = string_to_pair(var);
        if (p.first == main_type) {
            auto id = main_object->get_values(p.second);
            remote->append_values(main_type + "." + p.second, id, true);
        }
    }

    // Make sure there's a special "__related__" attribute created so
    // orphan filtering will work even if the above code didn't transfer
    // any attributes. This shouldn't be needed if we start using "real"
    // relations instead of copying attributes like this.
    const auto related_name = "__related__";
    string_vector related_value = {"1"};
    main_object->add_attribute(remote_type + "." + related_name, related_value);
    remote->add_attribute(main_type + "." + "__related__", related_value);
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
    if (relations.empty()) {
        return;
    }
    // Put all the required relations first so we don't recursively load related objects
    // if one of the required relations is missing.
    auto is_required = [](const relation& r){ return is_true(r.require); };
    std::stable_partition(relations.begin(), relations.end(), is_required);

    string_vector scim_vars = conf.get_vector_sorted_unique(type + "-scim-variables");

    indented_logger::indenter indenter(load_logger);

    std::map<std::string, std::set<std::string>> missing_local_values;
    const int MAX_MISSING_LOCAL_VALUES = 100;
    std::vector<std::string> to_remove;
    
    for (auto &&main_object: *objects) {
        for (auto &&relation: relations) {
            load_logger.log("Finding " + relation.type + " objects for " + type + " (" + readable_id(main_object.second.get(), type) + ")");
            indented_logger::indenter indenter(load_logger);
            bool warn_missing = is_true(relation.warn_missing);
            bool require = is_true(relation.require);
            bool relation_found = false;
            if (relation.method == "object") {
                auto relation_source = data_server::instance().get_by_type(relation.type);
                if (relation_source) {
                    string_vector values = main_object.second->get_values(relation.local_attribute);
                    for (size_t i = 0; i < values.size(); ++i) {
                        auto remote_object = server.find_object_by_attribute(relation.type,
                                                                             relation.remote_attribute, values[i]);
                        if (remote_object) {
                            relation_found = true;
                            establish_relation(main_object.second, remote_object, type, relation.type, load_logger);
                        }
                        else {
                            if (warn_missing && missing_local_values[relation.type].size() < MAX_MISSING_LOCAL_VALUES) {
                               missing_local_values[relation.type].insert(values[i]);
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
                            auto transform = get_transformer(relation.type);
                            auto limiter = get_limiter(relation.type);
                            auto response = ldap_to_object_list(ldap, relation.type, transform, limiter, load_logger);
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
                        relation_found = true;
                        establish_relation(main_object.second, remote, type, relation.type, load_logger);
                    }
                    else {
                        if (warn_missing && missing_local_values[relation.type].size() < MAX_MISSING_LOCAL_VALUES) {
                            missing_local_values[relation.type].insert(value);
                        }
                    }
                }
            }
            if (require && !relation_found) {
                to_remove.push_back(main_object.first);
                break; // don't go through the other relations
            }
        }
    }

    // Remove the objects which didn't establish a required relation
    for (const auto &uid : to_remove) {
        objects->remove(uid);
    }

    // Summarize local values for which we couldn't establish a relation (if warn_missing is used for the relation)
    for (const auto &relation: relations) {
        auto itr = missing_local_values.find(relation.type);
        if (itr != missing_local_values.end()) {
            std::cerr << "Local attribute values for which we couldn't establish a relation from " << type << " to " << relation.type << ":" << std::endl;
            const auto& values = itr->second;
            for (auto value : values) {
                std::cerr << value << std::endl;
            }
        }
    }
}

bool warn_if_bad_uuid(const std::string& uuid) {
    // These will be read from config file if available,
    // initialized to defaults if not in config file.
    static bool discard_objects_with_bad_uuids = false;
    static bool disable_bad_uuid_warnings = false;

    // Read from config file on first call
    static bool got_config_variables = false;
    if (!got_config_variables) {
        const config_file &config = config_file::instance();

        const auto disable_warnings_variable_name = "disable-bad-uuid-warnings";
        if (config.has(disable_warnings_variable_name)) {
            disable_bad_uuid_warnings = config.get_bool(disable_warnings_variable_name);
        }

        const auto discard_objects_variable_name = "discard-objects-with-bad-uuids";
        if (config.has(discard_objects_variable_name))  {
            discard_objects_with_bad_uuids = config.get_bool(discard_objects_variable_name);
        }
        got_config_variables = true;
    }

    for (auto ch : uuid) {
        if (isupper(ch)) {
            if (!disable_bad_uuid_warnings) {
                std::cerr << "Upper case character found in UUID: " << uuid << std::endl;
            }
            return !discard_objects_with_bad_uuids;
        }
    }
    const auto strict_pattern = "^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$";
    static std::regex* matcher = 0;

    if (matcher == 0) {
        matcher = new std::regex(strict_pattern, std::regex::optimize);
    }

    if (!std::regex_match(uuid, *matcher)) {
        if (!disable_bad_uuid_warnings) {
            std::cerr << "UUID with bad format: " << uuid << std::endl;
        }
        return !discard_objects_with_bad_uuids;
    }

    return true;
}
