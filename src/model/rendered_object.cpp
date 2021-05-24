/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2021 Föreningen Sambruk
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

#include "rendered_object.hpp"

rendered_object::rendered_object(const std::string &id,
                                 const std::string &type,
                                 const std::string &json) 
                                 : id(id), type(type), json(json) {
}

std::string rendered_object::get_id() const {
    return id;
}

std::string rendered_object::get_type() const {
    return type;
}

std::string rendered_object::get_json() const {
    return json;
}
