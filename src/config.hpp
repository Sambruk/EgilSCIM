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

#ifndef EGILSCIM_CONFIG_HPP
#define EGILSCIM_CONFIG_HPP

namespace config {

char csv_separator();
char csv_quote();

/** Should the load log include lines for when we skip objects?
 *  For instance due to load limiting, orphans filtering or 
 *  required relations.
 */
bool load_log_include_skipped();

/** Should we skip detecting duplicate uuids? */
bool ignore_duplicate_uuids();

/** Timeout setting (seconds) for the connection phase of the HTTP request */
int http_connection_timeout();

/** Timeout setting (seconds) for the whole HTTP request (including connection) */
int http_request_timeout();

/** The maximum number of timeouts we accept before we stop making
  *  more requests.
  */
int http_max_acceptable_timeouts();

/** Should variable expansions be escaped for JSON by default?
 *  If true, a variable expansion like ${foo} will escape foo's value,
 *  in that case ${|foo} can be used to disable escaping for a specific variable.
 *  If false, variables are not escaped unless the ${|foo} syntax is used.
 */
bool escape_expansions_by_default();

} // namespace config

#endif // EGILSCIM_CONFIG_HPP
