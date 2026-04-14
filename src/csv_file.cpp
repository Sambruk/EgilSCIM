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

#include "csv_file.hpp"
#include <fstream>
#include <cassert>

csv_file::csv_file(const std::string& path, char separator, char quote)
        : SEPARATOR(separator),
          QUOTE(quote) {
    std::ifstream fs(path);

    if (!fs) {
        throw std::runtime_error(std::string("failed to open file: ") + path);
    }

    load(fs);
}

csv_file::csv_file(std::istream& is, char separator, char quote)
        : SEPARATOR(separator),
          QUOTE(quote) {
    load(is);
}

namespace {

static const std::string UTF8_BOM("\xEF\xBB\xBF", 3);

void strip_utf8_bom(std::string& s) {
    if (s.size() >= 3 && s.substr(0, 3) == UTF8_BOM) {
        s.erase(0, 3);
    }
}

bool end_of_record(std::istream& is) {
    return (is.peek() == EOF || is.peek() == '\r' || is.peek() == '\n');
}

void eat_newline(std::istream& is) {
    auto bad = false;
    auto ch = is.get();
    if (ch == '\r') {
        ch = is.get();
        if (ch != '\n') {
            bad = true;
        }
    }
    else if (ch != EOF && ch != '\n') {
        bad = true;
    }

    if (bad) {
        throw csv_file::format_error("unrecognized line ending");
    }
}

std::string parse_non_escaped_field(std::istream& is, const char SEPARATOR, const char QUOTE) {
    std::string result;

    while (!end_of_record(is) && is.peek() != SEPARATOR) {
        if (is.peek() == QUOTE) {
            throw csv_file::format_error("unexpected quote in non-quoted field");
        }
        result += (char)is.get();
    }
    return result;
}

std::string parse_escaped_field(std::istream& is, const char QUOTE) {
    auto ch = is.get();
    assert(ch == QUOTE);
    
    std::string result;
    do {
        ch = is.get();
        
        if (ch == QUOTE) {
            if (is.peek() == QUOTE) {
                is.get();
                result += (char)ch;
            }
            else {
                return result;
            }
        }
        else if (ch != EOF) {
            result += (char)ch;
        }
    } while (ch != EOF);
    
    throw csv_file::format_error("unexpected end-of-file in quoted field");
}

std::string parse_field(std::istream& is, const char SEPARATOR, const char QUOTE) {
    if (end_of_record(is)) {
        return "";
    }

    if (is.peek() == QUOTE) {
        return parse_escaped_field(is, QUOTE);
    }
    return parse_non_escaped_field(is, SEPARATOR, QUOTE);
}

csv_file::row parse_record(std::istream& is, const char SEPARATOR, const char QUOTE) {
    csv_file::row result;
    auto field = parse_field(is, SEPARATOR, QUOTE);
    result.push_back(field);
    
    while (!end_of_record(is)) {
        auto ch = is.get();
        if (ch != SEPARATOR) {
            throw csv_file::format_error(std::string("expected '") + SEPARATOR + "', got " + (char)ch);
        }
        field = parse_field(is, SEPARATOR, QUOTE);
        result.push_back(field);
    }
    eat_newline(is);
    return result;
}

}

void csv_file::load(std::istream& is) {
    header = parse_record(is, SEPARATOR, QUOTE);
    if (!header.empty()) {
        // Somewhat ugly hack to support UTF-8 BOM. If there's a BOM in the file, 
        // it will be at the start of the first header field.
        // Strip it if it's there, but otherwise leave the header field unchanged.
        // Note that in the unlikely event that the file has a BOM and the first
        // header field is quoted, we'll get an exception in parse_record above
        // (because the BOM bytes will appear before the opening quote), but that's 
        // an acceptable limitation (which means we don't need to make any assumptions
        // about the istream supporting putback or seek, which would be needed if
        // we wanted to get rid of the BOM before parse_record).
        strip_utf8_bom(header[0]);
    }

    while (is.good() && is.peek() != EOF) {
        auto record_number = data.size() + 1;

        try {
            data.push_back(parse_record(is, SEPARATOR, QUOTE));
            if (data.back().size() != header.size()) {
                throw format_error("incorrect number of fields in record");
            }
        } catch (format_error& e) {
            throw format_error(std::string(e.what()) + ", record: " + std::to_string(record_number));
        }
    }
}

csv_file::row csv_file::get_header() const {
    return header;
}

size_t csv_file::size() const {
    return data.size();
}

csv_file::row csv_file::operator[](size_t i) const {
    return data[i];
}
