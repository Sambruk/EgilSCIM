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

#include "thresholds.hpp"
#include <sstream>
#include "config_file.hpp"

void verify_thresholds_for_type(int old_count, 
                                int new_count,
                                std::optional<int> absolute_threshold,
                                std::optional<int> relative_threshold) {
    if (!absolute_threshold.has_value() && !relative_threshold.has_value()) {
        return;
    }

    auto diff = std::abs(old_count - new_count);

    if (absolute_threshold.has_value() && diff > *absolute_threshold) {
        std::ostringstream os;
        os << "threshold exceeded (old count: " 
           << old_count 
           << ", new count: " 
           << new_count 
           << ", threshold: " 
           << *absolute_threshold << ")";
        throw threshold_error(os.str());
    }

    if (relative_threshold.has_value() && diff > *relative_threshold * 0.01 * old_count) {
        std::ostringstream os;
        os << "threshold exceeded (old count: " 
           << old_count 
           << ", new count: " 
           << new_count 
           << ", threshold (relative): " 
           << *relative_threshold << ")";
        throw threshold_error(os.str());
    }
}

/** Gets the threshold to use for a given type, from the config file.
 *  This function is used both for absolute and relative thresholds, send
 *  in either the suffix "-threshold" or "-threshold-relative" to determine
 *  which config file parameters to use at.
 * 
 *  If there isn't a type specific threshold, we will look for a generic 
 *  fallback (such as Object-threshold). If neither exist an optional without
 *  value is returned.
 * 
 *  Throws a runtime_error if the config parameters couldn't be parsed.
 */
std::optional<int> get_threshold(const std::string& type, const std::string& suffix) {
    config_file &config = config_file::instance();
    const std::string fallback_type("Object");

    try {
        if (config.has(type + suffix)) {
            return std::stoi(config.get(type + suffix));
        } else if (config.has(fallback_type + suffix)) {
            return std::stoi(config.get(fallback_type + suffix));
        } else {
            return std::optional<int>();
        }
    } catch (const std::logic_error& e) {
        throw std::runtime_error(std::string("failed to parse threshold: ") + e.what());
    }
}

std::optional<int> get_absolute_threshold(const std::string& type) {
    const auto suffix = "-threshold";
    return get_threshold(type, suffix);
}

std::optional<int> get_relative_threshold(const std::string& type) {
    const auto suffix = "-threshold-relative";
    return get_threshold(type, suffix);
}

int count_objects_of_type(std::shared_ptr<rendered_object_list> cache, const std::string& type) {
    if (cache == nullptr) {
        return 0;
    }
    int c = 0;
    for (const auto& itr : *cache) {
        if (itr.second->get_type() == type) {
            ++c;
        }
    }
    return c;
}

void verify_thresholds(std::shared_ptr<rendered_object_list> cache, const data_server& server) {
    auto types_to_verify = config_file::instance().get_vector("scim-type-send-order");

    for (auto& type : types_to_verify) {
        std::optional<int> absolute_threshold(get_absolute_threshold(type));
        std::optional<int> relative_threshold(get_relative_threshold(type));

        int old_count = count_objects_of_type(cache, type);
        auto current = server.get_by_type(type);
        int new_count = current == nullptr ? 0 : current->size();

        verify_thresholds_for_type(old_count, new_count, absolute_threshold, relative_threshold);
    }
}
