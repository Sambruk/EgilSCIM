/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2020 Föreningen Sambruk
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

#include "sql_load.hpp"
#include "sql.hpp"
#include "config_file.hpp"
#include "load_limiter.hpp"
#include "load_common.hpp"
#include <boost/property_tree/json_parser.hpp>

struct sql_aux_settings {
    std::string query;
};

struct sql_settings {
    std::string query;
    std::vector<sql_aux_settings> aux;
};

sql_settings parse_sql_settings(const std::string &json) {
    sql_settings result;

    namespace pt = boost::property_tree;   
    try {
        std::stringstream json_stream;
        json_stream << json;
        pt::ptree root;
        pt::read_json(json_stream, root);
        result.query = root.get<std::string>("query");

        auto aux = root.find("aux");

        if (aux != root.not_found()) {
            for (auto itr = aux->second.begin(); itr != aux->second.end(); ++itr) {
                auto node = itr->second;
                auto query = node.get<std::string>("query");
                result.aux.push_back({query});
            }
        }
    } catch (const pt::ptree_error &e) {
        throw std::runtime_error("failed to parse SQL settings");
    }

    return result;
}

// Goes through all records in an SQL result table and creates base_objects 
std::shared_ptr<object_list> sql_to_object_list(std::shared_ptr<sql::plugin::iterator> itr,
                                                const std::string& type) {
    auto objects =  std::make_shared<object_list>();
    const auto attribute_names = itr->get_header();
    
    std::vector<std::optional<std::string>> row;
    while (itr->next(row)) {
        auto object = vector_to_base_object(row, attribute_names, type);

        if (config_file::instance().has(type + "-UUID-generator")) {
            generate_uuid(object,
                          config_file::instance().get(type + "-UUID-generator"),
                          config_file::instance().get(type + "-unique-identifier"));
        }

        auto uid = object->get_uid();
        if (!uid.empty()) {
            objects->add_object(uid, object);
        }
    }

    return objects;
}

// Adds multi values attributes to a list of objects.
//
// itr is expected to iterate over a table with values for
// one multi-valued attribute.
void add_multi_valued(std::shared_ptr<sql::plugin::iterator> itr,
                      std::shared_ptr<object_list>& objects) {
    auto header = itr->get_header();

    // We expect exactly 2 columns, a key to the main table and
    // a column of values. The header for the first column will
    // determine which attribute in the main table to use for key.
    // The header for the second column will determine what the
    // multi valued attribute will be named.
    if (header.size() != 2) {
        throw std::runtime_error("expected exactly two columns in SQL table with multi valued attributes");
    }
    
    auto key = header[0];
    auto attribute = header[1];

    // Create an index on key
    std::map<std::string, std::shared_ptr<base_object>> index;

    for (auto& iter : *objects) {
        index[iter.second->get_values(key)[0]] = iter.second;
    }

    std::vector<std::optional<std::string>> row;
    while (itr->next(row)) {

        if (!row[0] || index.find(row[0].value()) == index.end()) {
            // TODO: Should we give a warning or something?
            continue;
        }

        if (row[1]) {
            index[row[0].value()]->append_values(attribute, {row[1].value()});
        }
    }
}

std::shared_ptr<object_list> sql_get(std::shared_ptr<sql::plugin> plugin,
                                     const std::string &type,
                                     indented_logger &load_logger) {
    load_logger.log(std::string("Loading entries for type ") + type + " with SQL");
    indented_logger::indenter indenter(load_logger);
    
    std::shared_ptr<object_list> objects;

    auto sql_settings_json = config_file::instance().get(type + "-sql");

    sql_settings settings;
    try {
        settings = parse_sql_settings(sql_settings_json);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(type + ":" + e.what());
    }

    // Create objects from the results of the main query
    try {
        {
            std::shared_ptr<sql::plugin::iterator> iterator(plugin->execute(settings.query));
            objects = sql_to_object_list(iterator, type);
        }

        // Get the multi-valued attributes
        for (size_t i = 0; i < settings.aux.size(); ++i) {
            auto aux = settings.aux[i];
            try {
                std::shared_ptr<sql::plugin::iterator> iterator(plugin->execute(aux.query));
                add_multi_valued(iterator, objects);
            } catch (const std::runtime_error& e) {
                throw std::runtime_error(type + " : " + e.what());
            }
        }

    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Failed to load " + type + " from SQL: " + e.what());
    }

    auto limiter = get_limiter(type);
    objects = filter_objects(objects, limiter, load_logger, type);

    load_related(type, objects, load_logger);

    return objects;
}
