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

#ifndef EGILSCIMCLIENT_GENERATED_LOAD_HPP
#define EGILSCIMCLIENT_GENERATED_LOAD_HPP

#include <memory>
#include <model/object_list.hpp>
#include "sql.hpp"
#include "utility/indented_logger.hpp"

std::shared_ptr<object_list> get_generated(const std::string& type,
                                           std::shared_ptr<sql::plugin> sql_plugin,
                                           indented_logger& load_logger);

using object_vector = std::vector<std::shared_ptr<base_object>>;

/** Deduce the test activity name suffix for a national test activity.
 * 
 *  The national test activities are named like GRGRMAT01_6 or SVASVA03_GY
 *  where the first part identifies a subject or course code and the second
 *  part is either a school year or a school type. If only the first part
 *  is available from the data source we can deduce the suffix if we can
 *  figure out the school type and school year based on the group's attributes
 *  and if necessary the group's students' school year attribute.
 * 
 *  @param deduce_from_school_type_attribute The attribute (in the group) to use for school type
 *  @param deduce_from_school_year_attribute The attribute (either in the group or in the students) to use for school year
 *  @param members_with_school_year If we want to use the students to deduce school year this is non-null.
 *  @returns for instance _6 or _GY.
 */
std::string deduce_suffix(std::shared_ptr<base_object> student_group, 
    const std::string& deduce_from_school_type_attribute, 
    const std::string& deduce_from_school_year_attribute,
    std::shared_ptr<object_vector> members_with_school_year);

#endif  // EGILSCIMCLIENT_GENERATED_LOAD_HPP
