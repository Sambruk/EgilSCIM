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

#include "transformer_impl.hpp"

multi_attribute_transformer::multi_attribute_transformer() {
}

void multi_attribute_transformer::add(std::shared_ptr<transformer> transform) {
    transformers.push_back(transform);
}

void multi_attribute_transformer::apply(base_object* obj) const {
    for (auto transform : transformers) {
        transform->apply(obj);
    }
}

void regex_transformer::apply(base_object* obj) const {
    string_vector values = obj->get_values(from);

    for (const auto& value : values) {
        bool foundMatch = false;
        for (auto rule : transforms) {
            if (std::regex_match(value, rule.match)) {
                auto transformed_value = std::regex_replace(value, rule.match, rule.replace, std::regex_constants::format_no_copy);
                obj->append_values(rule.to, {transformed_value});

                foundMatch = true;
                if (!matchAll) {
                    break;
                }
            }
        }

        if (!foundMatch && noMatch != "") {
            obj->append_values(noMatch, {value});
        }
    }
}
