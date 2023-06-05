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

#include <iostream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <assert.h>

#include "scim.hpp"
#include "utility/simplescim_error_string.hpp"
#include "utility/utils.hpp"
#include "model/base_object.hpp"
#include "model/object_list.hpp"
#include "config_file.hpp"
#include "cache_file.hpp"
#include "rendered_cache_file.hpp"
#include "simplescim_scim_send.hpp"
#include "readable_id.hpp"

// Concatenates a base URL with a path, for instance "https://foo.com" and "Users"
// into "https://foo.com/Users"
// If the first part ends with "/" or the second part starts with "/" we will make sure
// only one "/" is used (so the base URL in metadata can have either a trailing / or not,
// see #174).
// Since the fix for #174 technically changes behaviour, there's a configuration 
// variable for skipping the / cleaning. Hopefully we can remove this once we're sure
// no one actually needed that.
//
// When the legacy behaviour is gone we can also move this function to utils.cpp
// since it won't have a dependency on config_file.hpp anymore.
std::string concat_url(const std::string& s1, const std::string& s2) {
    bool legacy_behaviour = config_file::instance().get_bool("legacy-url-concatenation");
    auto first = s1;
    auto second = s2;

    if (!legacy_behaviour) {
        while (endsWith(first, "/")) {
            first = first.substr(0, first.size() - 1);
        }
        while (startsWith(second, "/")) {
            second = second.substr(1);
        }
    }
    return first + '/' + second;
}

int ScimActions::simplescim_scim_init() const {
    int err;

    /* Fetch variables from configuration file */
    const config_file &config = config_file::instance();
    auto cert = config.require_path("cert");
    auto key = config.require_path("key");

    if (cert.empty() || key.empty()) {
        return -1;
    }

    if (scim_new_cache == nullptr) {
        return -1;
    }

    /* Initialise simplescim_scim_send */
    err = scim_sender::instance().send_init(cert,
                                           key,
                                           scim_server_info.get_pinned_public_keys(),
                                           scim_server_info.get_ca_bundle_path());

    if (err == -1) {
        return -1;
    }

    return 0;
}

void ScimActions::simplescim_scim_clear() const {
    /* Clear simplescim_scim_send */
    scim_sender::instance().send_clear();
}

/**
 * Compares 'current' to 'cache' and performs 'copy_func'
 * on objects in both 'current' and cache if they are equal,
 * 'create_func' on objects in 'current' but not in
 * 'cache', performs 'update_func' on object in both
 * 'current' and 'cache' if the object has been updated.
 */
void ScimActions::process_changes(const object_list& current,
                                  const rendered_object_list &cache,
                                  const post_processing::plugins& ppp,
                                  statistics& stats,
                                  bool rebuild_cache,
                                  const std::set<std::string>& all_scim_uuids) const {
    int err;

    for (const auto &iter : current) {

        const std::string &uid = iter.first;
        const std::string readable = readable_id(iter.second.get());

        std::shared_ptr<rendered_object> object;
        try {
            object = rend.render(ppp, *iter.second);
        } catch (const std::runtime_error& e) {
            std::cerr << "Failed to render object to JSON: " << e.what() << std::endl;
        }
 
        std::shared_ptr<rendered_object> cached_object;
        bool create = false;

        if (rebuild_cache) {
            create = (all_scim_uuids.count(uid) == 0);
        }
        else {
            cached_object = cache.get_object(uid);
            create = (cached_object == nullptr);
        }

        if (create) {
            ++stats.n_create;
            if (object != nullptr) {
                auto create_functor = ScimActions::create_func(*object);
                err = create_functor(*this);
            } else {
                err = -1;
            }

            if (err == -1) {
                ++stats.n_create_fail;
                std::cerr << "Failed to create object " << readable << " of type " << iter.second->getSS12000type() << std::endl;
                if (object != nullptr) {
                    std::cerr << simplescim_error_string_get() << std::endl;
                }
            }
        } else {
            bool copy = false;

            if (rebuild_cache) {
                copy = false;
            }
            else {
                copy = (object == nullptr || (*object == *cached_object));
            }
            
            if (copy) {
                // Object is the same, copy it
                ++stats.n_copy;
                auto copy_functor = ScimActions::copy_func(*cached_object);
                if (copy_functor(*this) == -1) {
                    ++stats.n_copy_fail;
                    std::cerr << simplescim_error_string_get() << std::endl;
                }
            } else {
                ++stats.n_update;
                if (object != nullptr) {
                    ScimActions::update_func update_f(*object);
                    err = update_f(*this);
                }
                else {
                    err = -1;
                }
                
                if (err == -1) {
                    ++stats.n_update_fail;
                    std::cerr << "Failed to update object " << readable << " of type " << iter.second->getSS12000type() << std::endl;
                    if (object != nullptr) {
                        std::cerr << simplescim_error_string_get() << std::endl;
                    }
                }
            }
        }
    }
}

/*
 * Performs 'delete_func' on objects in 'cache' but not
 * in 'current'.
 */
void ScimActions::process_deletes(const object_list& current,
                                  const rendered_object_list& cache,
                                  const std::string& type,
                                  statistics& stats) const {

    /** For every object in 'cache' of the given type */
    for (const auto &item : cache) {
        std::shared_ptr<rendered_object> object = item.second;
        if (object->get_type() == type) {
            const std::string &uid = item.first;
            auto tmp = current.get_object(uid);

            if (tmp == nullptr) {
                // Object doesn't exist in 'current', delete it
                ++stats.n_delete;
                auto delete_f = ScimActions::delete_func(*object);
                int err = delete_f(*this);

                if (err == -1) {
                    ++stats.n_delete_fail;
                    fprintf(stderr, "%s\n", simplescim_error_string_get());
                }
            }
        }
    }
}

void ScimActions::process_deletes_per_endpoint(const std::vector<std::string>& to_delete,
                                               const std::string& endpoint,
                                               statistics& stats) const {
    auto prefix = concat_url(scim_server_info.get_url(), endpoint) + '/';

    for (const auto& uuid : to_delete) {
        ++stats.n_delete;

        const auto urlified = unifyurl(uuid);
        int err = scim_sender::instance().send_delete(prefix + urlified);
    
        if (err != 0) {
            ++stats.n_delete_fail;
            fprintf(stderr, "%s\n", simplescim_error_string_get());
        }
    }
}

void ScimActions::print_statistics(const std::string& type,
                                   const statistics& stats) {
    printf("Status:   Success   Failure     Total  of type: %s\n", type.c_str());
    printf("Copy:   %9zu %9zu %9zu\n", stats.n_copy - stats.n_copy_fail, stats.n_copy_fail, stats.n_copy);
    printf("Create: %9zu %9zu %9zu\n", stats.n_create - stats.n_create_fail, stats.n_create_fail, stats.n_create);
    printf("Update: %9zu %9zu %9zu\n", stats.n_update - stats.n_update_fail, stats.n_update_fail, stats.n_update);
    printf("Delete: %9zu %9zu %9zu\n", stats.n_delete - stats.n_delete_fail, stats.n_delete_fail, stats.n_delete);
}

/** This function will convert from SCIM endpoint (e.g. "SchoolUnits")
  * to SS12000 type (e.g. "SchoolUnit") if there is an unambiguous 
  * mapping between the two.
  *
  * For the "Users" endpoint, which could map to either "Student" or
  * "Teacher", this function will return "<Unqualified>".
  *
  * It is used to categorize the deletes in the statistics we present
  * after all SCIM operations have completed.
  *
  * @param endpoint is the endpoint to convert
  * @param types should include all SS12000 types used for this SCIM server.
  */
std::string endpoint_to_SS12000_type(const std::string& endpoint,
                                     const std::vector<std::string> types) {
    std::map<std::string, std::string> endpoint_to_SS12000;

    for (const auto& type : types) {
        auto url_param = type + "-scim-url-endpoint";
        if (!config_file::instance().has(url_param)) {
            continue;
        }
        auto ep = config_file::instance().get(url_param);

        if (endpoint_to_SS12000.find(ep) == endpoint_to_SS12000.end()) {
            endpoint_to_SS12000[ep] = type;
        }
        else {
            endpoint_to_SS12000[ep] = "<Unqualified>";
        }
    }

    if (endpoint_to_SS12000.find(endpoint) != endpoint_to_SS12000.end()) {
        return endpoint_to_SS12000[endpoint];
    }
    else {
        assert(false);
        return "<Unqualified>";
    }
}

int ScimActions::perform(const data_server &current,
                         const rendered_object_list &cached,
                         const post_processing::plugins& ppp,
                         bool rebuild_cache,
                         const std::vector<ScimActions::scim_object_ref>& all_scim_objects) const {
    std::string types_string = config_file::instance().get("scim-type-send-order");
    string_vector types = post_processing::filter_types(string_to_vector(types_string), ppp);

    std::map<std::string, statistics> stats;

    std::set<std::string> all_scim_uuids;
    if (rebuild_cache) {
        for (const auto& cur : all_scim_objects) {
            all_scim_uuids.insert(cur.uuid);
        }
    }
    
    for (const auto& type : types) {
        std::shared_ptr<object_list> allOfType = current.get_by_type(type);
        if (!allOfType) {
            allOfType = std::make_shared<object_list>();
        }

        process_changes(*allOfType, cached, ppp, stats[type], rebuild_cache, all_scim_uuids);
    }

    auto types_reversed(types);
    std::reverse(types_reversed.begin(), types_reversed.end());

    if (rebuild_cache) {
        std::set<std::string> endpoints;
        for (const auto& type : types_reversed) {
            auto url_param = type + "-scim-url-endpoint";
            if (!config_file::instance().has(url_param)) {
                continue;
            }
            std::string endpoint = config_file::instance().get(url_param);

            if (endpoints.count(endpoint)) {
                continue;
            }
            endpoints.insert(endpoint);

            std::vector<std::string> to_delete;

            for (const auto& obj : all_scim_objects) {
                if (obj.endpoint == endpoint &&
                    !current.has_object(obj.uuid)) {
                    to_delete.push_back(obj.uuid);
                }
            }
            
            process_deletes_per_endpoint(to_delete, endpoint, stats[endpoint_to_SS12000_type(endpoint, types)]);
        }
    }
    else {
        for (const auto& type : types_reversed) {
            std::shared_ptr<object_list> allOfType = current.get_by_type(type);
            if (!allOfType) {
                allOfType = std::make_shared<object_list>();
            }
            process_deletes(*allOfType, cached, type, stats[type]);        
        }
    }
    
    for (const auto& p : stats) {
        print_statistics(p.first, p.second);
    }

    /* Save new cache file */
    try {
        rendered_cache_file::save(config_file::instance().get_path("cache-file"), scim_new_cache);
    } catch (const std::runtime_error& e) {
        std::cerr << std::string("Failed to save cache file: ") + e.what() << std::endl;
        return -1;
    }

    return 0;
}

int ScimActions::copy_func::operator()(const ScimActions &actions) {
    if (cached.get_id().empty()) {
        return -1;
    }

    actions.scim_new_cache->add_object(std::make_shared<rendered_object>(cached));

    return 0;

}

int ScimActions::delete_func::operator()(const ScimActions &actions) {

    if (object.get_id().empty()) {
        simplescim_error_string_set_prefix("ScimActions::delete_func:"
                                           "get-attribute");
        simplescim_error_string_set_message("cached object does not have unique identifier attribute");
        return -1;
    }
    std::string url = actions.scim_server_info.get_url();
    std::string urlified = unifyurl(object.get_id());
    std::string endpoint = config_file::instance().get(object.get_type() + "-scim-url-endpoint");
    url = concat_url(url, endpoint);
    url = concat_url(url, urlified);

    /* Send SCIM delete request */
    int err = scim_sender::instance().send_delete(url);

    if (err != 0 && err != 404) { // Recache if delete failed, 404 is no failure
        actions.scim_new_cache->add_object(std::make_shared<rendered_object>(object));
    }
    if (err == 404) {
        simplescim_error_string_set_prefix("ScimActions::delete_func:");
        simplescim_error_string_set_message("tried to delete an object which the server says it doesn't have. Will not attempt to delete next run.");
    }

    return err;
}

int ScimActions::create_func::operator()(const ScimActions &actions) {
    std::string url = actions.scim_server_info.get_url();
    std::string endpoint = config_file::instance().get(create.get_type() + "-scim-url-endpoint");
    url = concat_url(url, endpoint);

    /* Send SCIM create request */
    bool conflict = false;
    std::optional<std::string> response_json =
        scim_sender::instance().send_create(url, create.get_json(), conflict);
    std::string id = create.get_id();
    if (response_json)
        actions.scim_new_cache->add_object(std::make_shared<rendered_object>(create));
    else {
        if (conflict) {
            // Put it in cache, but make sure we update in the next run
            auto copied_object = std::make_shared<rendered_object>(create.get_id(), create.get_type(), std::to_string(time(NULL)) + " (create conflict)");
            actions.scim_new_cache->add_object(copied_object);
        }
        return -1;
    }

    return 0;
}

int ScimActions::update_func::operator()(const ScimActions &actions) {
    std::string id = object.get_id();

    if (id.empty()) {
        return -1;
    }

    std::string unified = unifyurl(object.get_id());
    std::string url = actions.scim_server_info.get_url();
    std::string endpoint = config_file::instance().get(object.get_type() + "-scim-url-endpoint");
    url = concat_url(url, endpoint);
    url = concat_url(url, unified);

    bool non_existent = false;
    std::optional<std::string> response_json =
        scim_sender::instance().send_update(url, object.get_json(), non_existent);

    /* Insert copied object into new cache */
    if (!response_json) {
        if (!non_existent) {
            // Keep it in cache, but make sure we retry the update next run
            auto copied_object = std::make_shared<rendered_object>(object.get_id(), object.get_type(), std::to_string(time(NULL)) + " (failed update)");
            actions.scim_new_cache->add_object(copied_object);
        }
        return -1;
    } else {
        actions.scim_new_cache->add_object(std::make_shared<rendered_object>(object));
    }

    return 0;
}

std::vector<ScimActions::scim_object_ref> ScimActions::get_all_objects_from_scim_server() {
    config_file &config = config_file::instance();
    std::string types_string = config.get("scim-type-send-order");
    string_vector types = string_to_vector(types_string);

    std::set<std::string> endpoints;

    for (const auto& type : types) {
        auto url_param = type + "-scim-url-endpoint";
        if (config.has(url_param)) {
            endpoints.insert(config.get(url_param));
        }
    }

    std::vector<scim_object_ref> results;
    
    for (const auto& endpoint : endpoints) {
        std::vector<boost::property_tree::ptree> resources;
        std::string url = scim_server_info.get_url();
        url = concat_url(url, endpoint);
        scim_sender::instance().query(url, resources);

        for (const auto& resource : resources) {
            auto uuid = resource.get<std::string>("id");
            results.push_back(scim_object_ref(uuid, endpoint));
        }
    }

    return results;
}
