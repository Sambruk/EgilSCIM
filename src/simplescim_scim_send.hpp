/**
 * Copyright © 2017-2018  Max Wällstedt <>
 *
 * This file is part of EgilSCIM.
 *
 * EgilSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * EgilSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with EgilSCIM.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Further development with groups and relations support
 * by Ola Mattsson - IT informa for Sambruk
 */

#ifndef SIMPLESCIM_SCIM_SEND
#define SIMPLESCIM_SCIM_SEND

#include <string>
#include <optional>

class scim_sender {
public:
    static scim_sender instance() {
        static scim_sender sender;
        return sender;
    }

    /**
     * Initialises simplescim_scim_send.
     *
     * 'cert' must be the path of the client's certificate
     * file.
     *
     * 'pinnedpubkey' must be the sha256 hash of the server's
     * public key.
     *
     * On success, zero is returned. On error, -1 is returned
     * and simplescim_error_string is set to an appropriate
     * error message.
     */
    int send_init(std::string cert, std::string key, std::string pinnedpubkey);

    /**
     * Clears simplescim_scim_send and frees any associated
     * dynamically allocated memory.
     */
    void send_clear();

    /**
     * Sends a request to create a SCIM resource.
     *
     * 'url' must be of the format:
     * <protocol>://<host><endpoint>
     *
     * For example:
     * https://example.com/Users
     *
     * 'resource' must be the string representation of a JSON
     * object representing the SCIM resource.
     *
     * On success, zero is returned and 'response' is set to
     * the string representation of the JSON object returned by
     * the server. On error, -1 is returned and
     * simplescim_error_string is set to an appropriate error
     * message.
     */
    std::optional<std::string> send_create(const std::string &url, const std::string &body);

    /**
     * Sends a request to update a SCIM resource.
     *
     * 'url' must be of the format:
     * <protocol>://<host><endpoint>/<resource-identifier>
     *
     * For example:
     * https://example.com/Users/2819c223-7f76-453a-919d-413861904646
     *
     * 'resource' must be the string representation of a JSON
     * object representing the SCIM resource.
     *
     * On success, zero is returned and 'response' is set to
     * the string representation of the JSON object returned by
     * the server. On error, -1 is returned and
     * simplescim_error_string is set to an appropriate error
     * message.
     */
    std::optional<std::string> send_update(const std::string &url, const std::string &body);

    /**
     * Sends a request to delete a SCIM resource.
     *
     * 'url' must be of the format:
     * <protocol>://<host><endpoint>/<resource-identifier>
     *
     * For example:
     * https://example.com/Users/2819c223-7f76-453a-919d-413861904646
     *
     * On success, zero is returned. On error, -1 is returned
     * and simplescim_error_string is set to an appropriate
     * error message.
     */
    long send_delete(const std::string &url);
};

#endif
