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

#ifndef SIMPLESCIM_UTILS_HPP
#define SIMPLESCIM_UTILS_HPP

#include <cstdlib>
#include <string>
#include <vector>
#include <map>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

std::string unifyurl(const std::string &s);

std::vector<std::string> string_to_vector(const std::string &s);

std::pair<std::string, std::string> string_to_pair(const std::string &s);

std::string pair_to_string(const std::pair<std::string, std::string> &pair);

std::string toUpper(const std::string &s);

bool is_true(const std::string& s);

bool startsWith(const std::string& s, const std::string& prefix);
bool endsWith(const std::string& s, const std::string& suffix);

void print_error();

void print_status(const char *config_file_name);

class uuid_util
{
    boost::uuids::random_generator generator;

    uuid_util() = default;
public:
    static uuid_util &instance() {
        static uuid_util gen;
        return gen;
    }

    /**
     * Generates a UUID based on a string, in
     * a repeatable deterministic way.
     *
     * Technically we'll create a UUID version 5 (SHA-1)
     * from the string, with the standard UUID for object
     * identifiers as namespace.
     */
    std::string generate(const std::string& a);

    /**
     * Generates a UUID based on two strings, in
     * a repeatable deterministic way.
     *
     * This is the same as the version with just one string,
     * except the strings are are concatenated before hashing.
     */
    std::string generate(const std::string &a, const std::string &b);    

    // Parses a binary UUID, without any re-ordering of the bytes
    std::string un_parse_uuid(const void *val);

    // Parses a binary UUID in Microsoft format by first re-ordering some of the bytes
    std::string un_parse_ms_uuid(const void *val);
};

std::string format_log_path(const std::string& path);

/** Finds the most common element in a sequence.
 *  Will throw std::out_of_range if the sequence is empty.
 */
template<typename T, typename InputIterator>
T most_common(InputIterator begin, InputIterator end) {
    if (begin == end) {
        throw std::out_of_range("most_common: empty sequence");
    }
    std::map<T, int> counts;
    InputIterator current = begin;
    while (current != end) {
        counts[*current]++;
        ++current;
    }

    T result = *begin;
    int max = -1;

    for (auto p : counts) {
        if (p.second > max) {
            max = p.second;
            result = p.first;
        }
    }

    return result;
}

#endif //SIMPLESCIM_UTILS_HPP
