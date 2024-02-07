/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2024 Föreningen Sambruk
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

#ifndef EGILSCIM_SIMPLEURL_HPP
#define EGILSCIM_SIMPLEURL_HPP

#include <string>
#include <vector>

namespace simpleurl {
/** Wrapper over curl to perform a simple GET.
 *  Since curl is used the url can be for instance a file://-URL. The
 *  return code only has meaning if a GET is performed to a HTTP(S)-URL,
 *  in which case it is the HTTP status code.
 * 
 *  If the GET fails a runtime_error is thrown. A successful request where
 *  the response is for instance 404 Not Found is not considered a failure
 *  and will not throw, instead 404 is returned as the return value from
 *  this function.
 */
int read(const std::string& url, std::vector<char>& destination);
}

#endif // EGILSCIM_SIMPLEURL_HPP
