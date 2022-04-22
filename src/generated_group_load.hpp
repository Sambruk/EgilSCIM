/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2022 Föreningen Sambruk
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

#ifndef EGILSCIMCLIENT_GENERATED_GROUP_LOAD_HPP
#define EGILSCIMCLIENT_GENERATED_GROUP_LOAD_HPP

#include <memory>
#include <regex>
#include "utility/indented_logger.hpp"
#include "model/object_list.hpp"

struct student_group_attribute {
    std::string from;
    std::regex match;
    std::string uuid;
    std::vector<std::pair<std::string, std::string>> attributes;
};

std::vector<student_group_attribute> parse_student_group_attributes(const std::string& spec);

std::shared_ptr<object_list> get_generated_student_group(const std::string& type,
                                                         indented_logger& load_logger);

void add_scim_vars_for_virtual_groups();                                                         

#endif // EGILSCIMCLIENT_GENERATED_GROUP_LOAD_HPP
