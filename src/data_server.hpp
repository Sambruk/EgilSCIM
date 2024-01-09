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
#include "sql.hpp"

class data_server {
    // Loaded data, arranged by type
    std::map<std::string, std::shared_ptr<object_list>> data;

    std::unique_ptr<ldap_wrapper> ldap;
    std::unique_ptr<csv_store> csv;

    data_server(const data_server &other) = default;

    data_server() = default;

public:
    static data_server &instance() {
        static data_server s;
        return s;
    }

    void clear() {
        data.clear();
        ldap.reset();
        csv.reset();
    }

    bool empty() {
        return data.empty();
    }

    // Lookup of objects based on attribute value, meant to be used for attributes that can be used as primary key
    // so only one object is identified by the value. If multiple objects have the value for the given attribute
    // a random object will be returned. If no object is found a nullptr is returned.
    std::shared_ptr<base_object>
    find_object_by_attribute(const std::string &type, const std::string &attrib, const std::string &value);

    // Lookup of objects based on attribute value, returns multiple objects if many objects have the value for the given attribute.
    std::vector<std::shared_ptr<base_object>>
    find_objects_by_attribute(const std::string &type, const std::string &attrib, const std::string &value);

    bool has_object(const std::string& uuid) const;

    std::shared_ptr<object_list> get_by_type(const std::string &type) const;

    bool load(std::shared_ptr<sql::plugin> sql_plugin);

    void preload();

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
    void add_internal(const std::string &type, std::shared_ptr<object_list> list);

    void filter_orphans();

    bool is_orphan(const std::shared_ptr<base_object> object,
                   const std::vector<std::string> &attributes);

    indented_logger load_logger;
};


#endif //EGILSCIMCLIENT_DATA_SERVER_HPP
