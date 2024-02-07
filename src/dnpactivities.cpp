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

#include "dnpactivities.hpp"
#include <sstream>
#include <boost/property_tree/json_parser.hpp>
#include "utility/utils.hpp"
#include "utility/simpleurl.hpp"

namespace pt = boost::property_tree;

namespace {
// Parse a single test activity from a node in the JSON tree
DNPActivity parse_activity(const pt::ptree &node) {
    DNPActivity activity;
    activity.displayName = node.get<std::string>("displayName");
    activity.id = node.get<std::string>("id");
    return activity;
}
}

DNPActivities::DNPActivities(const std::string &data) {
    try {
        pt::ptree root;
        std::stringstream json_stream;
        json_stream << data;
        pt::read_json(json_stream, root);

        for (auto child : root) {
            auto activity = parse_activity(child.second);
            activities.push_back(activity);
        }
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("Failed to parse DNP activities: " + std::string(e.what()));
    }
}

boost::optional<std::string> DNPActivities::name_to_id(const std::string& name) const {
    for (size_t i = 0 ; i < activities.size(); ++i) {
        if (activities[i].displayName == name) {
            return activities[i].id;
        }
    }
    return boost::optional<std::string>();
}

std::shared_ptr<DNPActivities> create_activities_from_url(const std::string& url) {
    int http_code;
    std::vector<char> http_response;

    try {
        http_code = simpleurl::read(url, http_response);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error("failed to get activities from " + url + " : " + e.what());
    }

    if (startsWith(toUpper(url), "HTTP")) {
        if (http_code != 200) {
            throw std::runtime_error("unexpected HTTP response code for " + url + " : " + std::to_string(http_code));
        }
    }
    return std::make_shared<DNPActivities>(std::string(http_response.begin(), http_response.end()));
}
