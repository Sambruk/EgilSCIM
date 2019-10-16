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
#include <set>
#include <fstream>

// The null limiter includes everything
class null_limiter : public load_limiter {
public:
    virtual bool include(const base_object* obj) const {
        return true;
    }
};

/**
 * The list limiter includes only those objects with values
 * included in a text file.
 */
class list_limiter : public load_limiter {
public:
    list_limiter(const std::string& filename,
                 const std::string& attrib)
            : attribute(attrib) {
        load(filename);
    }

    virtual bool include(const base_object* obj) const {
        std::vector<std::string> values;
        if (attribute == "") {
            values.push_back(obj->get_uid());
        }
        else {
            values = obj->get_values(attribute);
        }

        for (const auto& value : values) {
            if (list.count(value) > 0) {
                return true;
            }
        }
        return false;
    }

    void load(const std::string filename) {
        std::ifstream ifs(filename);

        std::string value;
        while (ifs >> value) {
            list.insert(value);
        }
    }
    
private:
    std::set<std::string> list;
    const std::string attribute;
};

std::shared_ptr<load_limiter> get_limiter(const std::string& type) {
    config_file &conf = config_file::instance();

    if (conf.has(type + "-limit-with")) {
        auto limit_type = conf.get(type + "-limit-with");

        if (toUpper(limit_type) == "LIST") {
            auto filename = conf.get_path(type + "-limit-list");
            auto attribute = conf.get(type + "-limit-by", true);
            return std::make_shared<list_limiter>(filename, attribute);
        }
        else {
            throw std::runtime_error("No such limit type: " + limit_type);
        }
    }
    else {
        return std::make_shared<null_limiter>();
    }
}
