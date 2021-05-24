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

#ifndef EGILSCIM_RENDERED_OBJECT_LIST_HPP
#define EGILSCIM_RENDERED_OBJECT_LIST_HPP

#include "rendered_object.hpp"
#include <map>
#include <memory>

/**
 * A list of rendered objects.
 */
class rendered_object_list {
public:
    rendered_object_list() = default;
    void add_object(std::shared_ptr<rendered_object> obj);
    std::shared_ptr<rendered_object> get_object(const std::string& id);

private:
    std::map<std::string, std::shared_ptr<rendered_object>> objects;
};

#endif // EGILSCIM_RENDERED_OBJECT_LIST_HPP
