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

#ifndef EGILSCIMCLIENT_CSV_FILE_HPP
#define EGILSCIMCLIENT_CSV_FILE_HPP

#include <string>
#include <vector>
#include <istream>

/** A class for reading comma separated files according to
 *  RFC 4180.
 *
 *  RFC 4180 is rather restrictive, so we also allow:
 *
 *    - Unix style line endings
 *    - Configurable separator and quote character
 *    - UTF-8 encoded text
 *
 * The whole file is loaded and kept in memory.
 */
class csv_file {
public:
    // Load from a given file
    csv_file(const std::string& path, char separator = ',', char quote = '"');

    // Load from a stream
    csv_file(std::istream& is, char sep = ',', char q = '"');

    // A row in the file is a vector of strings
    typedef std::vector<std::string> row;

    // The first row in the file is interpreted as the header
    row get_header() const;

    // The number of rows in the file (not including the header)
    size_t size() const;

    // Returns one of the rows (i = 0 is the first data row, not the header)
    row operator[](size_t i) const;

    // A format_error is thrown by this class whenever the file is malformed
    class format_error : public std::runtime_error {
    public:
        format_error(const std::string what) : std::runtime_error(what) {}
    };

private:
    const char SEPARATOR;
    const char QUOTE;
    
    void load(std::istream& is);

    row header;
    std::vector<row> data;
};

#endif // EGILSCIMCLIENT_CSV_FILE_HPP
