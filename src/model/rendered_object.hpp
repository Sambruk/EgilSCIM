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

#ifndef EGILSCIM_RENDERED_OBJECT_HPP
#define EGILSCIM_RENDERED_OBJECT_HPP

#include <string>

/**
 * An object after it has been transformed to JSON form.
 * 
 * We keep type and id as separate members for easy access
 * after the object has been rendered.
 */
class rendered_object {
public:
    rendered_object(const std::string &id,
                    const std::string &type,
                    const std::string &json);

    std::string get_id() const;
    std::string get_type() const;
    std::string get_json() const;

    bool operator==(const rendered_object& other) const;

private:
    std::string id;
    std::string type;
    std::string json;
};

#endif // EGILSCIM_RENDERED_OBJECT_HPP