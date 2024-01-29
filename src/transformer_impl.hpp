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

#ifndef EGILSCIM_TRANSFORMER_IMPL_HPP
#define EGILSCIM_TRANSFORMER_IMPL_HPP

#include "transformer.hpp"
#include <regex>

class null_transformer : public transformer {
public:
    virtual void apply(base_object*) const {
    }
};

class multi_attribute_transformer : public transformer {
public:
    multi_attribute_transformer();
    virtual void apply(base_object* obj) const;
    void add(std::shared_ptr<transformer> transform);

private:
    std::vector<std::shared_ptr<transformer>> transformers;
};

struct regex_transform_rule {
    regex_transform_rule(const std::string& regex,
                         const std::string& to,
                         const std::string& replace)
        : match(regex, std::regex::optimize), to(to), replace(replace) {
    }

    std::regex match;
    std::string to;
    std::string replace;
};

class regex_transformer : public transformer {
public:
    regex_transformer(const std::string& from,
                      const std::vector<regex_transform_rule>& transforms,
                      bool matchAll,
                      const std::string& noMatch)
                      : from(from), transforms(transforms), matchAll(matchAll), noMatch(noMatch) {
    }

    virtual void apply(base_object* obj) const;

private:
    std::string from;
    std::vector<regex_transform_rule> transforms;
    bool matchAll;
    std::string noMatch;
};

class urldecode_transformer : public transformer {
public:
    urldecode_transformer(const std::string& from,
                          const std::string& to)
        : from(from), to(to) {
    }
    virtual void apply(base_object* obj) const;    

private:
    std::string from;
    std::string to;
};

#endif // EGILSCIM_TRANSFORMER_IMPL_HPP
