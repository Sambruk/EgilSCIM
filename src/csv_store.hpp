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

#ifndef EGILSCIMCLIENT_CSV_STORE_HPP
#define EGILSCIMCLIENT_CSV_STORE_HPP

#include <memory>
#include <map>
#include "csv_file.hpp"

/**
 *  The csv_store keeps csv files cached, so that our load process can
 *  visit the same csv file several time (only extracting individual
 *  entries each time) without having to reload and parse the files
 *  every time.
 */
class csv_store {
public:
    std::shared_ptr<csv_file> get_file(const std::string& path);

private:
    std::map<std::string, std::shared_ptr<csv_file>> cache;
};

#endif // EGILSCIMCLIENT_CSV_STORE_HPP
