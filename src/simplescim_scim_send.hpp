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

#ifndef SIMPLESCIM_SCIM_SEND
#define SIMPLESCIM_SCIM_SEND
#include <string>
#include <optional>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#endif
#include <curl/curl.h>
#include <fstream>
#include <vector>
#include <boost/property_tree/ptree_fwd.hpp>

class scim_sender {
public:
    static scim_sender& instance() {
        static scim_sender sender;
        return sender;
    }

    scim_sender()
    : number_of_timeouts(0), aborted(false) {
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
    int send_init(std::string cert,
                  std::string key,
                  std::string pinnedpubkey,
                  std::string ca_bundle_path);

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
     * 'body' must be the string representation of a JSON
     * object representing the SCIM resource.
     *
     * On success, the response from the server is returned,
     * this should be the string representation of the JSON
     * object returned by the server. On error, no string is
     * returned and simplescim_error_string is set to an 
     * appropriate error message.
     *
     * If the request failed due to the object already existing
     * on the server (409), conflict will be set to true.
     */
    std::optional<std::string> send_create(const std::string &url,
                                           const std::string &body,
                                           bool& conflict);

    /**
     * Sends a request to update a SCIM resource.
     *
     * 'url' must be of the format:
     * <protocol>://<host><endpoint>/<resource-identifier>
     *
     * For example:
     * https://example.com/Users/2819c223-7f76-453a-919d-413861904646
     *
     * 'body' must be the string representation of a JSON
     * object representing the SCIM resource.
     *
     * On success, the response from the server is returned,
     * this should be the string representation of the JSON 
     * object returned by the server. On error, no string is
     * returned and simplescim_error_string is set to an 
     * appropriate error message.
     *
     * If the request failed due to the object no longer existing
     * on the server (404), non_existent will be set to true.
     */
    std::optional<std::string> send_update(const std::string &url,
                                           const std::string &body,
                                           bool& non_existent);

    /**
     * Sends a request to delete a SCIM resource.
     *
     * 'url' must be of the format:
     * <protocol>://<host><endpoint>/<resource-identifier>
     *
     * For example:
     * https://example.com/Users/2819c223-7f76-453a-919d-413861904646
     *
     * If we didn't successfully perform an HTTP request, -1 is returned
     * and simplescim_error_string is set to an appropriate
     * error message. This could mean failed to connect or a timeout.
     * If a successful HTTP request was performed but we received a 
     * different response than HTTP 204, we will return the HTTP code
     * (such as 404 Not found).
     */
    long send_delete(const std::string &url);

    /**
     * Gets all resources for an endpoint. Handles pagination
     * if the server uses it (so this function may do several GETs
     * before it's done).
     *
     * Throws an std::runtime_error if there is a failure to
     * retrieve the resources.
     */
    void query(const std::string& url, std::vector<boost::property_tree::ptree>& resources);

    /** Sets the aborted state (see documentation for the aborted member below).
     *  Note that this class is not thread safe, this should not be called while
     *  another thread might be making requests.
     */
    void set_aborted() {
        aborted = true;
    }

    /** Returns the aborted state. See set_aborted().
     *  is_aborted is also not thread safe.
     */
    bool is_aborted() const {
        return aborted;
    }

private:

    void register_timeout();

    CURL *curl;

    /// Number of timeouts we've had so far
    int number_of_timeouts;

    /** Aborted means we shouldn't do any more real requests, just return errors.
     *  The aborted state is set if we've had too many timeouts when doing requests,
     *  if a request has failed in a way that implies later requests will also fail,
     *  or if surrounding code has asked us to go to the aborted state (typically
     *  because the program is being shut down and wants to exit prematurely).
     */
    bool aborted;

    std::ofstream http_log;
};

#endif
