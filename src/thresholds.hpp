/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2024 Föreningen Sambruk
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

#ifndef EGILSCIM_THRESHOLDS_HPP
#define EGILSCIM_THRESHOLDS_HPP

#include <memory>
#include "model/rendered_object_list.hpp"
#include "data_server.hpp"

/// threshold_error is thrown when a threshold is validated.
class threshold_error : public std::runtime_error {
public:
    threshold_error(const std::string& what) : std::runtime_error(what) {}
};

/** Verifies that the difference between old_count and new_count is within
 *  the configured limits.
 * 
 *  Throws threshold_error if a limit is exceeded.
 */
void verify_thresholds_for_type(int old_count,
                                int new_count,
                                std::optional<int> absolute_threshold,
                                std::optional<int> relative_threshold);

/** Verifies the thresholds for all types in scim-type-send-order. */
void verify_thresholds(std::shared_ptr<rendered_object_list> cache, const data_server& server);

/** Counts number of objects of a given type in the cache. */
int count_objects_of_type(std::shared_ptr<rendered_object_list> cache, const std::string& type);

#endif // EGILSCIM_THRESHOLDS_HPP