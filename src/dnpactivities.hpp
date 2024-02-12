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

#ifndef EGILSCIM_DNPACTIVITIES_HPP
#define EGILSCIM_DNPACTIVITIES_HPP

#include <string>
#include <vector>
#include <memory>
#include <boost/optional.hpp>

/** A single DNP test activity from Skolverket's API.
 *  For now we only care about the name and id.
 */
struct DNPActivity {
    std::string id;
    std::string displayName;
};

/** A list of DNP test activities.
 */
class DNPActivities {
public:
    /// Constructs the list based on the definition in the format read from Skolverket's API.
    DNPActivities(const std::string &data);

    /// Converts the name of an activity to its id (if it exists)
    boost::optional<std::string> name_to_id(const std::string& name) const;
private:
    std::vector<DNPActivity> activities;
};

/** Reads the test activities from a URL and parses them to create a 
 *  DNPActivities object.
 * 
 *  Throws runtime_error if the resource can't be fetched from the URL
 *  or if the data can't be parsed.
 */
std::shared_ptr<DNPActivities> create_activities_from_url(const std::string& url);

#endif // EGILSCIM_DNPACTIVITIES_HPP
