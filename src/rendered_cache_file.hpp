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

#ifndef EGILSCIM_RENDERED_CACHE_FILE_HPP
#define EGILSCIM_RENDERED_CACHE_FILE_HPP

#include <stdexcept>
#include <memory>
#include "model/rendered_object_list.hpp"

namespace rendered_cache_file {

class bad_format : public std::runtime_error {
public:
    bad_format() : std::runtime_error("unrecognized file format") {}
};

std::shared_ptr<rendered_object_list> get_contents(const std::string& path);

void save(const std::string& path, std::shared_ptr<rendered_object_list> objects);

}

#endif // EGILSCIM_RENDERED_CACHE_FILE_HPP
