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

#include "load_limiter.hpp"
#include "config_file.hpp"
#include "load_limiter_impl.hpp"

std::shared_ptr<load_limiter> create_limiter(const std::string& type) {
    config_file &conf = config_file::instance();

    if (conf.has(type + "-limit-with")) {
        auto limit_type = conf.get(type + "-limit-with");

        if (toUpper(limit_type) == "LIST") {
            auto filename = conf.get_path(type + "-limit-list");
            auto attribute = conf.get(type + "-limit-by", true);
            return std::make_shared<list_limiter>(filename, attribute);
        }
        else if (toUpper(limit_type) == "REGEX") {
            auto regex = conf.get(type + "-limit-regex");
            auto attribute = conf.get(type + "-limit-by");
            return std::make_shared<regex_limiter>(regex, attribute);
        }
        else {
            throw std::runtime_error("No such limit type: " + limit_type);
        }
    }
    else {
        return std::make_shared<null_limiter>();
    }
}

std::shared_ptr<load_limiter> get_limiter(const std::string& type) {
    config_file &conf = config_file::instance();

    static std::string current_config;
    static std::map<std::string, std::shared_ptr<load_limiter>> limiters;

    if (conf.file_name() != current_config) {
        limiters.clear();
        current_config = conf.file_name();
    }

    if (limiters.find(type) == limiters.end()) {
        limiters[type] = create_limiter(type);
    }
    return limiters[type];
}
