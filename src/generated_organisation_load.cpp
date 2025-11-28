/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2025 Föreningen Sambruk
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

#include "generated_organisation_load.hpp"
#include "config_file.hpp"

std::shared_ptr<object_list> get_generated_organisation(const std::string& type,
    indented_logger& load_logger) {
    config_file& conf = config_file::instance();

    auto uuid = conf.get(type + "-static-uuid");
    auto displayName = conf.get(type + "-display-name");

    auto generated = std::make_shared<object_list>();
    base_object generated_object(type);
    generated_object.add_attribute(conf.get(type + "-unique-identifier"), { uuid });
    generated_object.add_attribute("displayName", { displayName });
    generated->add_object(uuid, std::make_shared<base_object>(generated_object));

    load_logger.log(std::string("Generated ") + type + " " + displayName +
        " with id " + uuid);

    return generated;
}

void setup_default_organisation_config_variables() {
    auto default_template = R"(
{
  "schemas": ["urn:scim:schemas:extension:sis:school:1.0:Organisation"],
  "externalId": "${uuid}",
  "displayName": "${displayName}"
}
)";
    config_file& conf = config_file::instance();
    std::vector<std::tuple<std::string, std::string>> defaults({
        { "Organisation-scim-url-endpoint", "Organisations" },
        { "Organisation-unique-identifier", "uuid" },
        { "Organisation-scim-json-template", default_template},
        });

    for (auto& t : defaults) {
        auto& var = std::get<0>(t);
        auto& value = std::get<1>(t);
        if (!conf.has(var)) {
            conf.insert(var, value);
        }
    }
}