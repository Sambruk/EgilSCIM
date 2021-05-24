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

#ifndef EGILSCIM_RENDERER_HPP
#define EGILSCIM_RENDERER_HPP

#include "model/base_object.hpp"
#include "model/rendered_object.hpp"
#include "post_processing.hpp"

/**
 * A renderer can convert objects from the internal object model
 * to JSON form by applying templates and post processing.
 * 
 * The validity of the JSON generated from templates is only checked
 * once per type.
 */
class renderer {
public:
    /**
     * Renders an object to JSON form.
     * Throws runtime_error if there's a problem (for instance if the template
     * isn't generating valid JSON).
     */
    std::shared_ptr<rendered_object> render(const post_processing::plugins &ppp,
                                            const base_object& obj);

private:
    bool verify_json(const std::string & json, const std::string &type);
    string_vector verified_types;
};

#endif // EGILSCIM_RENDERER_HPP
