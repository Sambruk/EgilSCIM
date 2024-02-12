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
#include "csv_load.hpp"
#include "sql_load.hpp"
#include "json_data_file.hpp"

/**
 * load all data from
 * store each type in the data map with the type as key
 */
bool data_server::load(std::shared_ptr<sql::plugin> sql_plugin) {
    try {
        config_file &config = config_file::instance();

        std::shared_ptr<object_list> all = std::make_shared<object_list>();
        string_vector types = config.get_vector("scim-type-load-order");

        auto load_log_file = format_log_path(config_file::instance().get_path("load-log-file", true));
        if (load_log_file != "" && !load_logger.is_open()) {
            load_logger.open(load_log_file.c_str());
        }
        
        bool filtered_orphans = false;
        for (const auto &type : types) {
            std::shared_ptr<object_list> l;
            if (config.get_bool(type + "-is-generated")) {
                /* TODO
                 * At the moment we make sure to filter orphans before Activity and Employment,
                 * assuming those are at the end of the load order (so as to not generate
                 * activities and employments for objects which are orphans).
                 * Ideally we should probably instead do the filtering after everything is
                 * loaded. To get this to work properly we might need a multi-pass filtering,
                 * and perhaps proper relations.
                 */
                if (!filtered_orphans && (type == "Activity" || type == "Employment")) {
                    filter_orphans();
                    filtered_orphans = true;
                }
                l = get_generated(type, sql_plugin, load_logger);
            }
            else if (config.has(type + "-ldap-filter")) {
                auto ldap_wrapper = get_ldap_wrapper();
                if (ldap_wrapper->valid()) {
                    l = ldap_get(*ldap_wrapper, type, load_logger);
                }
                else {
                    std::cerr << "can't connect to LDAP" << std::endl;
                    return false;
                }
            }
            else if (config.has(type + "-csv-files")) {
                l = csv_get(type, load_logger);
            }
            else if (sql_plugin && config.has(type + "-sql")) {
                l = sql_get(sql_plugin, type, load_logger);
            }
            if (l) {
                add(type, l);
            }
            else {
                std::cerr << "load for " << type << " returned nothing" << std::endl;
            }
        }
        if (!filtered_orphans) {
            filter_orphans();
        }
    } catch (std::string msg) {
        return false;
    }
    preload();
    return true;
}

/**
 * An object is considered an orphan if it's missing all the
 * attributes given. For instance, a Student might be considered
 * an orphan if it's missing a StudentGroup attribute, or a
 * StudentGroup might be considered an orphan if it's missing
 * both Student and Teacher attributes.
 */
bool data_server::is_orphan(const std::shared_ptr<base_object> object,
                            const std::vector<std::string> &attributes) {
    for (const auto& attr : attributes) {
        if (object->has_attribute_or_relation(attr)) {
            return false;
        }
    }
    return true;
}

/**
 * Removes all objects considered orphans.
 */
void data_server::filter_orphans() {
    config_file &config = config_file::instance();
    string_vector types = config.get_vector("scim-type-load-order");

    for (const auto &data_for_type : data) {
        auto type = data_for_type.first;
        auto object_list = data_for_type.second;
        auto attributes = config.get_vector(type + "-orphan-if-missing", true);
        if (!attributes.empty()) {
            std::vector<std::string> to_remove;
            for (const auto &iter : *object_list) {
                if (is_orphan(iter.second, attributes)) {
                    to_remove.push_back(iter.second->get_uid());
                }
            }
            for (const auto &uid : to_remove) {
                object_list->remove(uid);
            }
        }
    }
}

void data_server::preload() {
    data_cache_vector caches = json_data_file::json_to_ldap_cache_requests(
            config_file::instance().get("user-caches", true));
}


/**
 * get all objects of type
 *
 * @param type the type of objects to return
 */
std::shared_ptr<object_list> data_server::get_by_type(const std::string &type) const {
    auto list = data.find(type);
    if (list != data.end()) {
        return list->second;
    }
    return nullptr;
}

void data_server::add_internal(const std::string &type, std::shared_ptr<object_list> list) {
    auto type_data = data.find(type);
    if (type_data == data.end())
        data.emplace(std::make_pair(type, list));
    else
        *type_data->second += *list;

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
    add_internal(type, list);
}

std::shared_ptr<base_object>
data_server::find_object_by_attribute(const std::string &type, const std::string &attrib, const std::string &value) {
    auto list = get_by_type(type);
    if (!list) {
        return nullptr;
    }
    return list->get_object_for_attribute(attrib, value);
}

std::vector<std::shared_ptr<base_object>>
data_server::find_objects_by_attribute(const std::string &type, const std::string &attrib, const std::string &value) {
    auto list = get_by_type(type);
    if (!list) {
        return std::vector<std::shared_ptr<base_object>>();
    }
    return list->get_objects_for_attribute(attrib, value);
}

bool data_server::has_object(const std::string& uuid) const {
    for (const auto& cur : data) {
        if (cur.second->get_object(uuid) != nullptr) {
            return true;
        }
    }
    return false;
}

std::shared_ptr<base_object> data_server::get_by_id(const std::string &type, const std::string &uuid) const {
    auto list = get_by_type(type);
    if (!list) {
        return nullptr;
    }
    return list->get_object(uuid);
}
