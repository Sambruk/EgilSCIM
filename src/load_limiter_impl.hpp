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

#ifndef EGILSCIM_LOAD_LIMITER_IMPL_HPP
#define EGILSCIM_LOAD_LIMITER_IMPL_HPP

#include <set>
#include <fstream>
#include <regex>
#include "load_limiter.hpp"

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

/**
 * The regex limiter includes only those objects with values
 * matching a regular expression.
 */
class regex_limiter : public load_limiter {
public:
    regex_limiter(const std::string& re,
                 const std::string& attrib)
            : attribute(attrib),
              expression(re) {
    }

    virtual bool include(const base_object* obj) const {
        auto values(obj->get_values(attribute));

        for (const auto& value : values) {
            if (std::regex_match(value, expression)) {
                return true;
            }
        }
        return false;
    }

private:
    const std::string attribute;
    const std::regex expression;
};

/**
 * The not limiter negates another limiter.
 */
class not_limiter : public load_limiter {
public:
    not_limiter(std::shared_ptr<load_limiter> l)
    : child(l) {}

    virtual bool include(const base_object* obj) const {
        return !child->include(obj);
    }

private:
    std::shared_ptr<load_limiter> child;
};

/**
 * The and limiter composes multiple limiters with
 * a logical AND operation.
 */
class and_limiter : public load_limiter {
public:
    and_limiter(std::vector<std::shared_ptr<load_limiter>> limiters)
    : children(limiters) {}

    virtual bool include(const base_object* obj) const {
        for (auto& child : children) {
            if (!child->include(obj)) {
                return false;
            }
        }
        return true;
    }

private:
    std::vector<std::shared_ptr<load_limiter>> children;
};

/**
 * The or limiter composes multiple limiters with
 * a logical OR operation.
 */
class or_limiter : public load_limiter {
public:
    or_limiter(std::vector<std::shared_ptr<load_limiter>> limiters)
    : children(limiters) {}

    virtual bool include(const base_object* obj) const {
        for (auto& child : children) {
            if (child->include(obj)) {
                return true;
            }
        }
        return false;
    }

private:
    std::vector<std::shared_ptr<load_limiter>> children;
};


#endif // EGILSCIM_LOAD_LIMITER_IMPL_HPP
