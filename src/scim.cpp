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
#include <algorithm>
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
#include "scim_json_parse.hpp"
#include "simplescim_scim_send.hpp"

namespace {

/*
 * This function converts from the type names used in the EGIL
 * client configuration to SS12000 types.
 *
 * Typically the type names are the same, except for Student and Teacher
 * which both map to User in SS12000.
 *
 * If the type has a config variable named `type`-SS12000-type, for instance:
 * 
 * Student-SS12000-type = User
 *
 * then that will be used. Otherwise type will be returned unchanged for 
 * everything except Student and Teacher.
 */
std::string actualSS12000type(const std::string& type) {
    auto config_var = type + "-SS12000-type";

    if (config_file::instance().has(config_var)) {
        return config_file::instance().get(config_var);
    }
    else {
        if (type == "Teacher" ||
            type == "Student") {
            return "User";
        }
        else {
            return type;
        }
    }
}

}

variables::variables() {
    const config_file &config = config_file::instance();
    variable_entries.emplace(std::make_pair("cert", config.require_path("cert")));
    variable_entries.emplace(std::make_pair("key", config.require_path("key")));
}


int ScimActions::simplescim_scim_init() const {
    int err;

    /* Fetch variables from configuration file */

    if (!vars.valid()) {
        return -1;
    }

    /* Allocate new cache */

    if (scim_new_cache == nullptr) {
        return -1;
    }

    /* Initialise simplescim_scim_send */
    err = scim_sender::instance().send_init(vars.get("cert"),
                                           vars.get("key"),
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

    /* Delete new cache */
    scim_new_cache->clear();
}

/**
 * Compares 'current' to 'cache' and performs 'copy_func'
 * on objects in both 'current' and cache if they are equal,
 * 'create_func' on objects in 'current' but not in
 * 'cache', performs 'update_func' on object in both
 * 'current' and 'cache' if the object has been updated.
 */
void ScimActions::process_changes(const object_list& current,
                                  const object_list &cache,
                                  const post_processing::plugins& ppp,
                                  statistics& stats,
                                  bool rebuild_cache,
                                  const std::set<std::string>& all_scim_uuids) const {
    int err;

    for (const auto &iter : current) {

        const std::string &uid = iter.first;

        auto object = iter.second;
        std::shared_ptr<base_object> cached_object;
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
            auto create_functor = ScimActions::create_func(*object);
            err = create_functor(*this, ppp);

            if (err == -1) {
                ++stats.n_create_fail;
                std::cerr << simplescim_error_string_get() << std::endl;
            }
        } else {
            bool copy = false;

            if (rebuild_cache) {
                copy = false;
            }
            else {
                copy = (*object == *cached_object);
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
                ScimActions::update_func update_f(*object);

                if (update_f(*this, ppp) == -1) {
                    ++stats.n_update_fail;
                    std::cerr << simplescim_error_string_get() << std::endl;
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
                                  const object_list& cache,
                                  const std::string& type,
                                  statistics& stats) const {

    /** For every object in 'cache' of the given type */
    for (const auto &item : cache) {
        std::shared_ptr<base_object> object = item.second;
        if (object->getSS12000type() == type) {
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
    auto prefix = scim_server_info.get_url() + '/' + endpoint + '/';

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
        auto ep = config_file::instance().get(type + "-scim-url-endpoint");

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
                         const object_list &cached,
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
            std::string endpoint = config_file::instance().get(type + "-scim-url-endpoint");

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
    int err = cache_file::instance().save(scim_new_cache);

    return err;
}

bool ScimActions::verify_json(const std::string & json, const std::string &type) const {
    if (json.empty())
        return false;
    else if (std::find(verified_types.begin(), verified_types.end(), type) != verified_types.end())
        return true;

    namespace pt = boost::property_tree;
    pt::ptree root;
    std::stringstream os;

    os << json;
    try {
        pt::read_json(os, root);
        verified_types.emplace_back(type);
    } catch (const pt::ptree_error& e) {
        std::cerr << "Failed to parse JSON for " << type << std::endl;
        simplescim_error_string_set_message(e.what());
        return false;
    }
    return true;
}

int ScimActions::copy_func::operator()(const ScimActions &actions) {
    if (cached.get_uid().empty()) {
        return -1;
    }

    actions.scim_new_cache->add_object(cached.get_uid(),
                                       std::make_shared<base_object>(cached));

    return 0;

}

int ScimActions::delete_func::operator()(const ScimActions &actions) {

    if (object.get_uid().empty()) {
        simplescim_error_string_set_prefix("ScimActions::delete_func:"
                                           "get-attribute");
        simplescim_error_string_set_message("cached object does not have unique identifier attribute");
        return -1;
    }
    std::string url = actions.scim_server_info.get_url();
    std::string urlified = unifyurl(object.get_uid());
    std::string endpoint = config_file::instance().get(object.getSS12000type() + "-scim-url-endpoint");
    url += '/' + endpoint + '/' + urlified;

    /* Send SCIM delete request */
    int err = scim_sender::instance().send_delete(url);

    if (err != 0 && err != 404) { // Recache if delete failed, 404 is no failure
        actions.scim_new_cache->add_object(object.get_uid(), std::make_shared<base_object>(object));
    }

    return err;
}

int ScimActions::create_func::operator()(const ScimActions &actions,
                                         const post_processing::plugins& ppp) {

    base_object copied_user(create);

    /* Create JSON object for object */
    std::string type = copied_user.getSS12000type();
    if (type == "base") {
        type = "User";
    }

    std::string standard_type = actualSS12000type(type);

    std::string template_json = actions.conf.get(type + "-scim-json-template");
    std::string parsed_json = scim_json_parse(template_json, copied_user);
    
    if (parsed_json == "") {
        std::cerr << "Failed to parse JSON template for " << type << std::endl;
        return -1;
    }

    if (!actions.verify_json(parsed_json, type))
        return -1;

    try {
        parsed_json = post_processing::process(ppp, standard_type, parsed_json);
    } catch (const std::runtime_error& e) {
        std::cerr << "Post processing error when creating object "
                  << copied_user.get_uid() << ": " << e.what();
        return -1;
    }

    std::string url = actions.scim_server_info.get_url();
    std::string endpoint = config_file::instance().get(type + "-scim-url-endpoint");
    url += '/' + endpoint;

    /* Send SCIM create request */
    bool conflict = false;
    std::optional<std::string> response_json =
        scim_sender::instance().send_create(url, parsed_json, conflict);
    std::string uid = copied_user.get_uid();
    if (response_json)
        actions.scim_new_cache->add_object(uid, std::make_shared<base_object>(copied_user));
    else {
        if (conflict) {
            // Put it in cache, but make sure we update in the next run
            copied_user.add_attribute("_failed_create", { std::to_string(time(NULL)) });
            actions.scim_new_cache->add_object(uid, std::make_shared<base_object>(copied_user));
        }
        return -1;
    }

    return 0;
}

int ScimActions::update_func::operator()(const ScimActions &actions,
                                         const post_processing::plugins& ppp) {

    /* Copy object */
    base_object copied_user(object);

    std::string uid = copied_user.get_uid();

    if (uid.empty()) {
        return -1;
    }

    /* Create JSON object for object */
    std::string type = object.getSS12000type();
    if (type == "base") {
        type = "User";
    }
    
    std::string standard_type = actualSS12000type(type);
    
    std::string create_var = type + "-scim-json-template";
    std::string template_json = config_file::instance().get(create_var);
    std::string parsed_json = scim_json_parse(template_json, copied_user);

    if (parsed_json == "") {
        std::cerr << "Failed to parse JSON template for " << type << std::endl;
        return -1;
    }
    
    if (!actions.verify_json(parsed_json, type)) {
        return -1;
    }

    try {
        parsed_json = post_processing::process(ppp, standard_type, parsed_json);
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Post processing error when updating object "
                  << copied_user.get_uid() << ": " << e.what();
        return -1;
    }

    std::string unified = unifyurl(object.get_uid());
    std::string url = actions.scim_server_info.get_url();
    std::string endpoint = config_file::instance().get(type + "-scim-url-endpoint");
    url += '/' + endpoint + '/' + unified;

    bool non_existent = false;
    std::optional<std::string> response_json =
        scim_sender::instance().send_update(url, parsed_json, non_existent);

    /* Insert copied object into new cache */
    if (!response_json) {
        if (!non_existent) {
            // Keep it in cache, but make sure we retry the update next run
            copied_user.add_attribute("_failed_put", { std::to_string(time(NULL)) });
            actions.scim_new_cache->add_object(uid, std::make_shared<base_object>(copied_user));
        }
        return -1;
    } else {
        actions.scim_new_cache->add_object(uid, std::make_shared<base_object>(copied_user));
    }

    return 0;

}

std::vector<ScimActions::scim_object_ref> ScimActions::get_all_objects_from_scim_server() {
    std::string types_string = config_file::instance().get("scim-type-send-order");
    string_vector types = string_to_vector(types_string);

    std::set<std::string> endpoints;

    for (const auto& type : types) {
        endpoints.insert(config_file::instance().get(type + "-scim-url-endpoint"));
    }

    std::vector<scim_object_ref> results;
    
    for (const auto& endpoint : endpoints) {
        std::vector<boost::property_tree::ptree> resources;
        std::string url = scim_server_info.get_url();
        url += '/' + endpoint;
        scim_sender::instance().query(url, resources);

        for (const auto& resource : resources) {
            auto uuid = resource.get<std::string>("id");
            results.push_back(scim_object_ref(uuid, endpoint));
        }
    }

    return results;
}
