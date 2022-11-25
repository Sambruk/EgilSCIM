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

#ifndef EGILSCIM_TRANSFORMER_HPP
#define EGILSCIM_TRANSFORMER_HPP

#include <string>
#include "model/base_object.hpp"
#include <memory>

/** 
 * The transformer is used to modify objects after they've been
 * read from the data source. Typically to transform certain attributes
 * according to rules (such as regex replacement).
 * 
 * Depending on transform rules in the config file, we'll
 * instantiate different sub-classes.
 */
class transformer {
public:
    virtual ~transformer() {}
    // Applies the transformation to (by modifying) the object
    virtual void apply(base_object* obj) const = 0;
};

std::shared_ptr<transformer> get_transformer(const std::string& type);
std::vector<std::string> get_transformed_attributes(const std::string& type);

#endif // EGILSCIM_TRANSFORMER_HPP
