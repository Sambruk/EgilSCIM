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
        if (separator_setting == "\\t") {
            separator = '\t';
        }
        else {
            separator = separator_setting[0];
        }
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

bool load_log_include_skipped() {
    return config_file::instance().get_bool("load-log-include-skipped");
}

bool ignore_duplicate_uuids() {
    return config_file::instance().get_bool("ignore-duplicate-uuids");
}

int http_connection_timeout() {
    return config_file::instance().get_int("http-connection-timeout", 30);
}

int http_request_timeout() {
    return config_file::instance().get_int("http-request-timeout", 120);
}

int http_max_acceptable_timeouts() {
    return config_file::instance().get_int("http-max-acceptable-timeouts", 3);
}

bool escape_expansions_by_default() {
    return config_file::instance().get_bool("escape-expansions-by-default");
}

} // namespace config
