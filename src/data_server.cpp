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

#include "data_server.hpp"
#include "config_file.hpp"
#include "generated_load.hpp"
#include "simplescim_ldap.hpp"
#include "json_data_file.hpp"

/**
 * load all data from
 * store each type in the data map with the type as key
 */
bool data_server::load() {
    try {
        config_file &config = config_file::instance();
        static_types = string_to_vector(config.get("scim-static-types"));
        dynamic_types = string_to_vector(config.get("scim-dynamic-types"));

        std::shared_ptr<object_list> all = std::make_shared<object_list>();
        string_vector types = config.get_vector("scim-type-load-order");
        struct closer {
            ldap_wrapper ldap;

            ldap_wrapper &get() {
                return ldap;
            }

            ~closer() {
                ldap.ldap_close();
            }

        } ldap;
        auto load_log_file = config_file::instance().get_path("load-log-file", true);
        if (load_log_file != "" && !load_logger.is_open()) {
            load_logger.open(load_log_file.c_str());
        }
        
        for (const auto &type : types) {
            std::shared_ptr<object_list> l;
            if (config.get_bool(type + "-is-generated")) {
                l = get_generated(type, load_logger);
            }
            else if (config.has(type + "-ldap-filter")) {
                if (ldap.get().valid()) {
                    l = ldap_get(ldap.get(), type, load_logger);
                }
                else {
                    std::cerr << "can't connect to ldap" << std::endl;                        
                }
            }
            else if (config.has(type + "-csv-files")) {

            }
            if (l) {
                add(type, l);
            }
            else {
                std::cerr << "load for " << type << " returned nothing" << std::endl;
            }
        }
    } catch (std::string msg) {
        return false;
    }
    preload();
    return true;
}


void data_server::preload() {
    data_cache_vector caches = json_data_file::json_to_ldap_cache_requests(
            config_file::instance().get("user-caches", true));
}


/**
 * get all objects of type
 *
 * @param type the type of objects to return
 * @param query the query used to load the data
 * @return
 */
std::shared_ptr<object_list> data_server::get_by_type(const std::string &type) const {
    auto list = static_data.find(type);
    if (list != static_data.end())
        return list->second;

    list = dynamic_data.find(type);
    if (list != dynamic_data.end())
        return list->second;

    return nullptr;
}

std::shared_ptr<object_list> data_server::get_static_by_type(const std::string &type) {
    auto stuff = static_data.find(type);
    if (stuff != static_data.end())
        return stuff->second;

    return std::make_shared<object_list>();
}


void data_server::add_dynamic(const std::string &type, std::shared_ptr<object_list> list) {
    auto type_data = dynamic_data.find(type);
    if (type_data == dynamic_data.end())
        dynamic_data.emplace(std::make_pair(type, list));
    else
        *type_data->second += *list;

}


void data_server::add_static(const std::string &type, std::shared_ptr<object_list> list) {
    static_data.emplace(std::make_pair(type, list));
}

void data_server::add(const std::string &type, std::shared_ptr<base_object> object) {
    auto list = get_by_type(type);
    if (list)
        list->add_object(object->get_uid(true), object);
    else {
        auto newList = std::make_shared<object_list>();
        newList->add_object(object->get_uid(true), object);
        add(type, newList);
    }
}


void data_server::add(const std::string &type, std::shared_ptr<object_list> list) {
    if (std::find(dynamic_types.begin(), dynamic_types.end(), type) != dynamic_types.end())
        add_dynamic(type, list);
    else
        add_static(type, list);
}

void data_server::cache_relation(const std::string &key, std::weak_ptr<base_object> object) {
    alt_key_cache.emplace(std::make_pair(key, object));
}

#define TEST_CACHE 0

std::shared_ptr<base_object>
data_server::find_object_by_attribute(const std::string &type, const std::string &attrib, const std::string &value) {

#if TEST_CACHE
    std::cout << type << " " << attrib << " " << value;
#endif
    auto found_pair = alt_key_cache.find(type + attrib + value);

    if (found_pair != alt_key_cache.end()) {
        if (auto sp = found_pair->second.lock()) {
#if TEST_CACHE
            std::cout << " Cache hit" << std::endl;
#endif
            return sp;
        }
#if TEST_CACHE
        else {
            std::cout << " Cached object invalid" << std::endl;
        }
#endif
    }

#if TEST_CACHE
    std::cout << " Cache miss";
#endif
    auto list = get_by_type(type);
    if (!list)
        return nullptr;
    auto result = list->get_object_for_attribute(attrib, value);
    if (result) {
#if TEST_CACHE
        std::cout << " - cached";
#endif
        cache_relation(type + attrib + value, result);
    }
#if TEST_CACHE
    else
        std::cout << " - not found";
    std::cout << std::endl;
#endif
    return result;
}


bool data_server::has_object(const std::string& uuid) const {
    for (const auto& cur : static_data) {
        if (cur.second->get_object(uuid) != nullptr) {
            return true;
        }
    }

    for (const auto& cur : dynamic_data) {
        if (cur.second->get_object(uuid) != nullptr) {
            return true;
        }
    }
    
    return false;
}
