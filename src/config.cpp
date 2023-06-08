/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2023 Föreningen Sambruk
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

#include "config.hpp"
#include "config_file.hpp"

namespace config {

char csv_separator() {
    char separator = ',';

    auto separator_setting = config_file::instance().get("csv-separator", true);
    if (!separator_setting.empty()) {
        separator = separator_setting[0];
    }
    return separator;
}

char csv_quote() {
    char quote = '"';

    auto quote_setting = config_file::instance().get("csv-quote", true);
    if (!quote_setting.empty()) {
        quote = quote_setting[0];
    }
    return quote;
}

} // namespace config