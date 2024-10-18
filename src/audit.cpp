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

#include "audit.hpp"
#include <sstream>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace audit {

const char *operation_to_string(bool success, SCIMOperation op) {
    switch (op)
    {
    case SCIM_CREATE:
        return success ? "Created" : "Failed to create";
    case SCIM_DELETE:
        return success ? "Deleted" : "Failed to delete";
    case SCIM_UPDATE:
        return success ? "Updated" : "Failed to update";
    default:
        return "<unknown operation>";
    }
}

const char* failure_type_to_string(SCIMOperationFailureType ft) {
    switch (ft)
    {
    case SCIM_OTHER_FAILURE:
        return "other";
    case SCIM_CONFLICT_FAILURE:
        return "conflict";
    case SCIM_NOT_FOUND_FAILURE:
        return "not found";
    default:
        return "<unknown failure type>";
    }
}

void log_scim_operation(std::ostream& os,
                       bool success,
                       SCIMOperationFailureType failure_type,
                       SCIMOperation operation,
                       const std::string &type,
                       const std::string &uuid,
                       std::shared_ptr<rendered_object> previous,
                       std::shared_ptr<rendered_object> current) {
    if (!os) {
        return;
    }

    std::time_t now = std::time(nullptr);

    std::tm tm;
#ifdef _WIN32
    localtime_s(&tm, &now);
#else
    localtime_r(&now, &tm);
#endif

    os << std::put_time(&tm, "%F %T") << " "
        << scim_operation_audit_message(success, failure_type, operation, type, uuid, previous, current) << std::endl;
}

std::string scim_operation_audit_message(bool success,
    SCIMOperationFailureType failure_type,
    SCIMOperation operation,
    const std::string& type,
    const std::string& uuid,
    std::shared_ptr<rendered_object> previous,
    std::shared_ptr<rendered_object> current) {
    std::ostringstream os;

    auto description = uuid;
    auto object_to_base_description_on = current;

    if (operation == SCIM_CREATE || operation == SCIM_UPDATE) {
        object_to_base_description_on = current != nullptr ? current : previous;
    }
    else {
        object_to_base_description_on = previous;
        assert(operation == SCIM_DELETE);
    }
    if (object_to_base_description_on != nullptr) {
        description = audit::object_description(object_to_base_description_on);
    }

    os << operation_to_string(success, operation);
    if (failure_type != SCIM_OTHER_FAILURE) {
        os << " (" << failure_type_to_string(failure_type) << ")";
    }
    os << " " << type << " " << description;
    return os.str();
}

// Assumes object is not a nullptr
std::string object_description(std::shared_ptr<rendered_object> object) {
    std::ostringstream os;
    os << object->get_id();

    // Try to parse JSON
    namespace pt = boost::property_tree;
    pt::ptree root;
    try {
        std::stringstream json_stream;
        json_stream << object->get_json();
        pt::read_json(json_stream, root);
    } catch (const pt::ptree_error&) {
        os <<  " (unparsable JSON)";
        return os.str();
    }

    // If there's a userName attribute, use that
    auto userName = root.get_optional<std::string>("userName");
    if (userName) {
        os << " " << userName.get();
        return os.str();
    }

    // Otherwise if there's a displayName, use that
    auto displayName = root.get_optional<std::string>("displayName");
    if (displayName) {
        os << " " << displayName.get();
        auto owner = root.get_optional<std::string>("owner.value");
        if (owner) {
            os << " owner: " << owner.get();
        }
        return os.str();
    }

    // Otherwise if type == Employment, use UUIDs for user and school unit
    if (object->get_type() == "Employment") {
        auto user = root.get_optional<std::string>("user.value");
        auto schoolUnit = root.get_optional<std::string>("employedAt.value");
        if (user) {
            os << " user: " << user.get();
        }
        if (schoolUnit) {
            os << " employed at: " << schoolUnit.get(); 
        }
        return os.str();
    }

    return os.str();
}

} // namespace audit