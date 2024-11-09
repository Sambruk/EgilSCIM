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

#ifndef EGILSCIM_AUDIT_HPP
#define EGILSCIM_AUDIT_HPP

#include <string>
#include <memory>
#include <ostream>
#include "model/rendered_object.hpp"
#include "scim.hpp"

namespace audit {

/**
 * Logs an message as a result of a SCIM operation (either a successful or
 * failed operation).
 * The log message is meant to be readable and will follow a somewhat 
 * predictable pattern, but is not completely structured for easy parsing.
 * 
 * The function will do nothing if os is not an opened stream.
 * 
 * previous refers to a version of the object which had been sent before,
 * current refers to the current version. 
 * 
 * Whether or not current and previous are nullptrs will partly depend on
 * which operation was performed, but there are exceptions (for instance
 * if the operation failed because the current object couldn't be rendered,
 * or if the operation was done during a "rebuild cache").
 * 
 * In other words the log message, when describing the object, will try to
 * use the most suitable object, or in the worst case just the UUID.
 * 
 * To describe the objects, they will be parsed if possible and we'll use
 * SS12000 knowledge to choose an appropriate readable description for the 
 * object (for instance, for users we will use the userName).
 * 
 */
void log_scim_operation(std::ostream& os,
                        bool success,
                        SCIMOperationFailureType failure_type,
                        SCIMOperation operation,
                        const std::string &type,
                        const std::string &uuid,
                        std::shared_ptr<rendered_object> previous,
                        std::shared_ptr<rendered_object> current);

/** 
* Testable implementation of log_scim_operation.
* Returns the message in a string instead of writing to a stream if it's open.
* Doesn't add a timestamp to the start of the message.
*/
std::string scim_operation_audit_message(bool success,
    SCIMOperationFailureType failure_type,
    SCIMOperation operation,
    const std::string& type,
    const std::string& uuid,
    std::shared_ptr<rendered_object> previous,
    std::shared_ptr<rendered_object> current);

/**
 * Describes a rendered object. This function assumes object is not a nullptr.
 * The description will attempt to find a readable description that identifies
 * the object. It will always start with the UUID so the object can be uniquely
 * identified, but it will also add a more user friendly attribute if possible.
 * To choose a user friendly attribute, this function uses knowledge of SS12000.
 */
std::string object_description(std::shared_ptr<rendered_object> object);

} // namespace audit

#endif // EGILSCIM_AUDIT_HPP
