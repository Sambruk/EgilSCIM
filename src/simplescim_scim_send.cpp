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

#include "simplescim_scim_send.hpp"

#include <stdlib.h>
#include <string.h>
#include <string>
#include <iostream>
#include <boost/property_tree/json_parser.hpp>

#include "utility/simplescim_error_string.hpp"
#include "config_file.hpp"

namespace pt = boost::property_tree;

struct http_response {
    size_t len;
    size_t alloc;
    char *data;
};

std::string simplescim_scim_send_cert;
std::string simplescim_scim_send_key;
std::string simplescim_scim_send_pinnedpubkey;
std::string simplescim_scim_send_ca_bundle_path;

static char simplescim_scim_send_errbuf[CURL_ERROR_SIZE];

static void simplescim_scim_send_print_curl_error(const char *function, CURLcode errnum) {
    size_t len;

    /* Set prefix */

    simplescim_error_string_set_prefix("%s", function);

    /* Set message */

    len = strlen(simplescim_scim_send_errbuf);

    if (len == 0) {
        simplescim_error_string_set_message("%s", curl_easy_strerror(errnum));
    } else {
        if (simplescim_scim_send_errbuf[len - 1] == '\n') {
            simplescim_scim_send_errbuf[len - 1] = '\0';
        }

        simplescim_error_string_set_message("%s", simplescim_scim_send_errbuf);
    }
}

static size_t simplescim_scim_send_write_func(void *ptr, size_t size, size_t nmemb, void *userdata) {
    struct http_response *http_response;
    size_t len;
    size_t i;
    char *tmp;
    char c;

    http_response = static_cast<struct http_response *>(userdata);
    len = size * nmemb;

    for (i = 0; i < len; ++i) {
        c = ((char *) ptr)[i];

        if (c == '\r') {
            continue;
        }

        if (http_response->len + 1 == http_response->alloc) {
            tmp = static_cast<char *>(realloc(http_response->data, http_response->alloc * 2));

            if (tmp == nullptr) {
                return i;
            }

            http_response->data = tmp;
            http_response->alloc *= 2;
        }

        http_response->data[http_response->len] = c;
        ++http_response->len;
    }

    http_response->data[http_response->len] = '\0';

    return len;
}

static std::string http_header(const std::string& header, const std::string& value) {
    return header + ": " + value;
}

static struct curl_slist *simplescim_scim_send_create_slist(const std::string &method) {
    struct curl_slist *chunk, *tmp_chunk;

    std::string media_type = config_file::instance().get("scim-media-type", true);
    if (media_type.empty()) {
        media_type = "application/scim+json";
    }
    
    if ((method == "POST") || (method == "PUT")) {
        tmp_chunk = curl_slist_append(nullptr, http_header("Accept", media_type).c_str());

        if (tmp_chunk == nullptr) {
            simplescim_error_string_set("simplescim_scim_send_create_slist", "curl_slist_append() returned nullptr");
            return nullptr;
        }

        chunk = curl_slist_append(tmp_chunk, http_header("Content-Type", media_type).c_str());

        if (chunk == nullptr) {
            simplescim_error_string_set("simplescim_scim_send_create_slist", "curl_slist_append() returned nullptr");
            curl_slist_free_all(tmp_chunk);
            return nullptr;
        }

        return chunk;
    }
    else if (method == "DELETE") {
        chunk = curl_slist_append(nullptr, "Accept:");

        if (chunk == nullptr) {
            simplescim_error_string_set("simplescim_scim_send_create_slist", "curl_slist_append() returned nullptr");
            return nullptr;
        }

        return chunk;
    }
    else if (method == "GET") {
        chunk = curl_slist_append(nullptr, http_header("Accept", media_type).c_str());

        if (chunk == nullptr) {
            simplescim_error_string_set("simplescim_scim_send_create_slist", "curl_slist_append() returned nullptr");
            return nullptr;
        }
        return chunk;
    }
    else {
        simplescim_error_string_set("simplescim_scim_send_create_slist", "invalid HTTP method");
        return nullptr;
    }
}

static int simplescim_scim_send(CURL* curl,
                                const std::string &url, const std::string &resource,
                                const std::string &method, char **response_data, long *response_code,
                                std::ofstream& http_log) {

    CURLcode errnum;
    struct curl_slist *chunk;
    struct http_response http_response{};
    long http_code;

    /* Enable more elaborate error messages */

    errnum = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, simplescim_scim_send_errbuf);

    if (errnum != CURLE_OK) {
        simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_ERRORBUFFER)", errnum);
        return -1;
    }

    bool auth = !config_file::instance().get_bool("scim-auth-WEAK");
    /* host verification */
    if (auth) {
        errnum = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    } else {
        errnum = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        errnum = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    }

    // CURLE_BAD_FUNCTION_ARGUMENT is returned when setting CURLOPT_SSL_VERIFYHOST 1, odd IMHO
    if (errnum != CURLE_OK && errnum != CURLE_BAD_FUNCTION_ARGUMENT) {
        simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_SSL_VERIFYHOST)", errnum);
        return -1;
    }

    /* Set certificate */
    errnum = curl_easy_setopt(curl, CURLOPT_SSLCERT, simplescim_scim_send_cert.c_str());
    if (errnum != CURLE_OK) {
        simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_SSLCERT)", errnum);
        return -1;
    }

    /* Set private key */

    errnum = curl_easy_setopt(curl, CURLOPT_SSLKEY, simplescim_scim_send_key.c_str());

    if (errnum != CURLE_OK) {
        simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_SSLKEY)", errnum);
        return -1;
    }

    if (auth) {
        /* ca cert from skolfederation metadata */
        errnum = curl_easy_setopt(curl, CURLOPT_CAINFO, simplescim_scim_send_ca_bundle_path.c_str());

        if (errnum != CURLE_OK) {
            simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_CAINFO)", errnum);
            return -1;
        }
    }

    if (auth) {
        /* Set pinned public key */
        errnum = curl_easy_setopt(curl, CURLOPT_PINNEDPUBLICKEY, simplescim_scim_send_pinnedpubkey.c_str());
        
        if (errnum != CURLE_OK) {
            simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_PINNEDPUBLICKEY)", errnum);
            return -1;
        }
    }
    
    static bool verbose_curl = config_file::instance().get_bool("verbose_logging");
    errnum = curl_easy_setopt(curl, CURLOPT_VERBOSE, verbose_curl);

    if (errnum != CURLE_OK) {
        simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_VERBOSE)", errnum);
        return -1;
    }


    /* Set HTTP method */

    errnum = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());

    if (errnum != CURLE_OK) {
        simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_CUSTOMREQUEST)", errnum);
        return -1;
    }

    /* Set URL */

    errnum = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

    if (errnum != CURLE_OK) {
        simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_URL)", errnum);
        return -1;
    }

    /* Set SCIM resource */

    if (!resource.empty()) {
        errnum = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, resource.c_str());

        if (errnum != CURLE_OK) {
            simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_POSTFIELDS)", errnum);
            return -1;
        }
    }

    /* Set empty body for DELETE operations */
    
    if (method == "DELETE") {
        errnum = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, "");

        if (errnum != CURLE_OK) {
            simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_POSTFIELDS)", errnum);
            return -1;
        }        
    }

    /* Set HTTP headers for SCIM */

    chunk = simplescim_scim_send_create_slist(method);

    if (chunk == nullptr) {
        return -1;
    }

    errnum = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

    if (errnum != CURLE_OK) {
        simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_HTTPHEADER)", errnum);
        curl_slist_free_all(chunk);
        return -1;
    }

    /* Set write callback */

    if (response_data != nullptr) {
        errnum = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, simplescim_scim_send_write_func);

        if (errnum != CURLE_OK) {
            simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_WRITEFUNCTION)", errnum);
            curl_slist_free_all(chunk);
            return -1;
        }
    }

    /* Set data pointer */

    if (response_data != nullptr) {
        http_response.len = 0;
        http_response.alloc = 1024;
        http_response.data = static_cast<char *>(malloc(http_response.alloc));
        *response_data = http_response.data;
        if (http_response.data == nullptr) {
            simplescim_error_string_set_errno("simplescim_scim_send:"
                                              "malloc");
            curl_slist_free_all(chunk);
            return -1;
        }

        http_response.data[0] = '\0';

        errnum = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &http_response);

        if (errnum != CURLE_OK) {
            simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_WRITEDATA)", errnum);
            free(http_response.data);
            curl_slist_free_all(chunk);
            return -1;
        }
    }

    /* Perform request */

    errnum = curl_easy_perform(curl);

    if (errnum != CURLE_OK) {
        simplescim_scim_send_print_curl_error("curl_easy_perform", errnum);

//        if (response_data != nullptr) {
//            free(http_response.data);
//        }

        curl_slist_free_all(chunk);
        if (errnum == CURLE_COULDNT_CONNECT || errnum == CURLE_SSL_CERTPROBLEM ||
            errnum == CURLE_SSL_CACERT_BADFILE || errnum == CURLE_SSL_CACERT ||
            errnum == CURLE_SSL_PINNEDPUBKEYNOTMATCH) {
            throw std::string();
        }
        return -1;
    }

    if (http_log) {
        http_log << ">>>>>>>>>>\n";
        http_log << method << " to " << url;
        if (!resource.empty()) {
            http_log << " with body:\n" << resource;
        }
        http_log << "\n";
        http_log << ">>>>>>>>>>\n";
    }
    
    /* Get response code */

    errnum = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (errnum != CURLE_OK) {
        simplescim_scim_send_print_curl_error("curl_easy_getinfo", errnum);

        if (response_data != nullptr) {
            free(http_response.data);
        }

        curl_slist_free_all(chunk);
        return -1;
    }

    /* Clean up */

    if (response_data != nullptr) {
        *response_data = http_response.data;
    }

    *response_code = http_code;

    if (http_log) {
        http_log << "<<<<<<<<<<\n";
        http_log << "Got reply with HTTP code " << http_code;
        if (response_data != nullptr) {
            http_log << " and body:\n" << http_response.data;
        }
        http_log << "\n";
        http_log << "<<<<<<<<<<\n";       
    }    

    curl_slist_free_all(chunk);

    return 0;
}

/**
 * Initialises simplescim_scim_send.
 *
 * 'cert' must be the path of the client's certificate
 * file.
 *
 * 'pinnedpubkey' must be the sha256 hash of the server's
 * public key in base64 encoding.
 *
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int scim_sender::send_init(std::string cert,
                           std::string key,
                           std::string pinnedpubkey,
                           std::string ca_bundle_path) {
    CURLcode errnum;

    errnum = curl_global_init(CURL_GLOBAL_DEFAULT);

    if (errnum != 0) {
        simplescim_scim_send_print_curl_error("curl_global_init", errnum);
        return -1;
    }

    /* Initialise curl session */

    curl = curl_easy_init();

    if (curl == nullptr) {
        simplescim_error_string_set("curl_easy_init", "curl_easy_init() returned nullptr");
        return -1;
    }    

    simplescim_scim_send_cert = cert;
    simplescim_scim_send_key = key;
    simplescim_scim_send_pinnedpubkey = pinnedpubkey;
    simplescim_scim_send_ca_bundle_path = ca_bundle_path;

    auto http_log_file = config_file::instance().get_path("http-log-file", true);
    if (http_log_file != "" && !http_log.is_open()) {
        http_log.open(http_log_file, std::ios_base::out | std::ios_base::trunc);
    }

    return 0;
}

/**
 * Clears simplescim_scim_send and frees any associated
 * dynamically allocated memory.
 */
void scim_sender::send_clear() {
    curl_global_cleanup();
    curl_easy_cleanup(curl);
}

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
std::optional<std::string> scim_sender::send_create(const std::string &url, const std::string &body) {
    char *response_data;
    long response_code;
    int err;

    err = simplescim_scim_send(curl, url, body, "POST", &response_data, &response_code, http_log);

    std::string res;
    if (err == -1) {
        free(response_data);
        return {};
    } else if (response_code < 500) {
        res = response_data;
    }

    if (response_code != 201 && response_code != 200) {
        simplescim_error_string_set_prefix("simplescim_scim_send_create");
        std::string message;
        if (response_code == 409) {
            message = " object already exists";
            simplescim_error_string_set_message("HTTP response code %ld returned, expected %ld %s", response_code, 201L,
                                                message.c_str());
        } else if (response_code == 413) {
            message = " data to " + url + " to large.";
            simplescim_error_string_set_message("HTTP response code %ld returned, expected %ld %s", response_code, 201L,
                                                message.c_str());
            return {};
        } else if (response_code == 403) {
            message = " unathorized ";
            simplescim_error_string_set_message("HTTP response code %ld returned, expected %ld %s", response_code, 201L,
                                                message.c_str());
            return {};
        } else {
            std::cerr << res << std::endl;
            message = url;
            simplescim_error_string_set_message("HTTP response code %ld returned, expected %ld %s", response_code, 201L,
                                                message.c_str());
            return {};

        }

    }
    std::string result(response_data);
    free(response_data);

    return result;
}

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
std::optional<std::string>
scim_sender::send_update(const std::string &url, const std::string &body) {
    char *response_data;
    long response_code;
    int err;

    err = simplescim_scim_send(curl, url, body, "PUT", &response_data, &response_code, http_log);

    if (err == -1) {
        free(response_data);
        return {};
    }

    if (response_code != 200) {
        simplescim_error_string_set_prefix("simplescim_scim_send_update");
        simplescim_error_string_set_message("HTTP response code %ld returned, expected %ld", response_code, 200L);
        free(response_data);
        return {};
    }
    std::string result{response_data};
    free(response_data);
    return result;
}

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
long scim_sender::send_delete(const std::string &url) {
    long response_code;
    int err;

    err = simplescim_scim_send(curl, url, "", "DELETE", nullptr, &response_code, http_log);

    if (err == -1) {
        return -1;
    }

    if (response_code != 204) {
        simplescim_error_string_set_prefix("simplescim_scim_send_delete");
        simplescim_error_string_set_message("HTTP response code %ld returned, expected %ld", response_code, 204L);
        return response_code;
    } else if (response_code == 413) {
        std::string message = " data to " + url + " to large.";
        simplescim_error_string_set_message("HTTP response code %ld returned, expected %ld %s", response_code, 201L,
                                            message.c_str());
        return {};
    } else if (response_code == 403) {
        std::string message = " unathorized ";
        simplescim_error_string_set_message("HTTP response code %ld returned, expected %ld %s", response_code, 201L,
                                            message.c_str());
        return {};
    }

    return 0;
}

/**
  * A function type for performing a GET.
  * Used in order to mock the http traffic in unit tests.
  */
typedef std::function<int(const std::string& url, char **response_data, long *response_code)> GetFunc;

/**
 * This is the actual implementation for querying a SCIM server for
 * all resources for an endpoint.
 *
 * scim_sender::query has the clean interface, whereas this function
 * allows for mocking the http GET and has a parameter for start_Index
 * so it can recursively call itself to continue fetching.
 */
void simplescim_query_impl(const std::string& url, std::vector<pt::ptree>& resources, GetFunc getter, int start_index = 1) {
    char *response_data;
    long response_code;

    auto url_complete{url};
    if (start_index != 1) {
        url_complete += "?startIndex=" + std::to_string(start_index);
    }
    
    int err = getter(url_complete, &response_data, &response_code);

    if (err == -1) {
        throw std::runtime_error(simplescim_error_string_get());
    }

    if (response_code != 200) {
        throw std::runtime_error("Failed to GET " + url + ", HTTP response code " + std::to_string(response_code) + " returned, expected 200");
    }

    std::string response(response_data);
    std::istringstream iss(response);
    pt::ptree root;
    pt::read_json(iss, root);

    auto totalResults = root.get<int>("totalResults");

    if (totalResults == 0) {
        return;
    }

    auto itr = root.find("Resources");
    
    if (itr == root.not_found()) {
        throw std::runtime_error("No Resources list in query result");
    }

    auto page_size = 0;
    for (const auto& cur : itr->second) {
        resources.push_back(cur.second);
        ++page_size;
    }

    if (static_cast<int>(resources.size()) < totalResults) {
        simplescim_query_impl(url, resources, getter, start_index + page_size);
    }
}

void scim_sender::query(const std::string& url, std::vector<pt::ptree>& resources) {
    auto curl_getter =
        [this](const std::string& url, char **response_data, long *response_code) -> int {
        return simplescim_scim_send(curl, url, "", "GET", response_data, response_code, http_log);
    };
    
    simplescim_query_impl(url, resources, curl_getter);
}
