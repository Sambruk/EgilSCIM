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

#ifndef EGILSCIMCLIENT_DATA_SERVER_HPP
#define EGILSCIMCLIENT_DATA_SERVER_HPP


#include <memory>
#include <set>
#include "model/object_list.hpp"
#include "utility/indented_logger.hpp"
#include "ldap_wrapper.hpp"
#include "csv_store.hpp"

class data_server {
    // static data is loaded once. They are known full sets like SchoolUnit
    // All are loaded initially.
    // Also generated object like Employment and Activity are static.
    std::map<std::string, std::shared_ptr<object_list>> static_data;
    // dynamic data is objects like User, they are loaded as a consequence of loading
    // groups and generated data.
    // The distinction is if we can say "it is loaded or not" v.s. it grows as we load
    // other data
    std::map<std::string, std::shared_ptr<object_list>> dynamic_data;

    // when relational objects are loaded they will later be requested by
    // another key than GUID.
    // This requires a search of all objects by attribute. This cache intercepts that and
    // is indexed by <type><attribute><id>. It holds weak pointers
    std::map<std::string, std::weak_ptr<base_object>> alt_key_cache;

    string_vector static_types;
    string_vector dynamic_types;

    std::unique_ptr<ldap_wrapper> ldap;
    std::unique_ptr<csv_store> csv;

    data_server(const data_server &other) = default;

    data_server() = default;

    bool isStatic(const std::string &type) const {
        for (auto && v : static_types) {
            if (v == type)
                return true;
        }
        return false;
    }

public:
    static data_server &instance() {
        static data_server s;
        return s;
    }

    void clear() {
        static_types.clear();
        dynamic_types.clear();
        static_data.clear();
        dynamic_data.clear();
        ldap.reset();
        csv.reset();
    }

    bool empty() {
        return static_data.empty() && dynamic_data.empty();
    }

//	std::shared_ptr<base_object>
//	get_object_by_attribute(const std::string &type, const std::string &attrib, const std::string &value);
    std::shared_ptr<base_object>
    find_object_by_attribute(const std::string &type, const std::string &attrib, const std::string &value);

    bool has_object(const std::string& uuid) const;

    std::shared_ptr<object_list> get_by_type(const std::string &type) const;

    /**
     * dependent data that is loaded in its entirety once
     * @param type
     * @return
     */
    std::shared_ptr<object_list> get_static_by_type(const std::string &type);


    bool load();

    void preload();

    void cache_relation(const std::string& key, std::weak_ptr<base_object> object);

    void add(const std::string& type, std::shared_ptr<object_list> list);

    void add(const std::string &type, std::shared_ptr<base_object> object);

    ldap_wrapper* get_ldap_wrapper() {
        if (ldap.get() == nullptr) {
            ldap.reset(new ldap_wrapper());
        }
        return ldap.get();
    }

    csv_store* get_csv_store() {
        if (csv.get() == nullptr) {
            csv.reset(new csv_store());
        }
        return csv.get();
    }

private:
    /**
     * static_data is loaded once
     * @param type
     * @param list
     */
    void add_static(const std::string &type, std::shared_ptr<object_list> list);

    /**
     * add dynamic data, this is appended to as static data is loaded
     * @param type
     * @param list
     */
    void add_dynamic(const std::string &type, std::shared_ptr<object_list> list);

    indented_logger load_logger;
};


#endif //EGILSCIMCLIENT_DATA_SERVER_HPP
