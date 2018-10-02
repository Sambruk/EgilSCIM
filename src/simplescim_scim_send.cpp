#include <utility>

#include <utility>

/**
 * Copyright © 2017-2018  Max Wällstedt <>
 *
 * This file is part of SimpleSCIM.
 *
 * SimpleSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with SimpleSCIM.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Further development with groups and relations support
 * by Ola Mattsson - IT informa for Sambruk
 */

#include "simplescim_scim_send.hpp"

#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <string>
#include <iostream>

#include "utility/simplescim_error_string.hpp"
#include "config_file.hpp"

struct http_response {
	size_t len;
	size_t alloc;
	char *data;
};

std::string simplescim_scim_send_cert;
std::string simplescim_scim_send_key;
std::string simplescim_scim_send_pinnedpubkey;

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

static struct curl_slist *simplescim_scim_send_create_slist(const std::string &method) {
	struct curl_slist *chunk, *tmp_chunk;

	if ((method == "POST") || (method == "PUT")) {
		tmp_chunk = curl_slist_append(nullptr, "Accept: application/scim+json");

		if (tmp_chunk == nullptr) {
			simplescim_error_string_set("simplescim_scim_send_create_slist", "curl_slist_append() returned nullptr");
			return nullptr;
		}

//		chunk = curl_slist_append(tmp_chunk, "Content-Type: application/scim+json");
		chunk = curl_slist_append(tmp_chunk, "Content-Type: application/json");

		if (chunk == nullptr) {
			simplescim_error_string_set("simplescim_scim_send_create_slist", "curl_slist_append() returned nullptr");
			curl_slist_free_all(tmp_chunk);
			return nullptr;
		}

		return chunk;
	} else if (method == "DELETE") {
		chunk = curl_slist_append(nullptr, "Accept:");

		if (chunk == nullptr) {
			simplescim_error_string_set("simplescim_scim_send_create_slist", "curl_slist_append() returned nullptr");
			return nullptr;
		}

		return chunk;
	} else {
		simplescim_error_string_set("simplescim_scim_send_create_slist", "invalid HTTP method");
		return nullptr;
	}
}

static int simplescim_scim_send(const std::string &url, const std::string &resource,
                                const std::string &method, char **response_data, long *response_code) {

	bool self_assigned_cert = !config_file::instance().get_bool("server-cert-using-CA");

	CURL *curl;
	CURLcode errnum;
	struct curl_slist *chunk;
	struct http_response http_response{};
	long http_code;

	/* Initialise curl session */

	curl = curl_easy_init();

	if (curl == nullptr) {
		simplescim_error_string_set("curl_easy_init", "curl_easy_init() returned nullptr");
		return -1;
	}

	/* Enable more elaborate error messages */

	errnum = curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, simplescim_scim_send_errbuf);

	if (errnum != CURLE_OK) {
		simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_ERRORBUFFER)", errnum);
		curl_easy_cleanup(curl);
		return -1;
	}

	/* Disable peer verification */
	if (self_assigned_cert) {
		errnum = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);

		if (errnum != CURLE_OK) {
			simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_SSL_VERIFYPEER)", errnum);
			curl_easy_cleanup(curl);
			return -1;
		}
	}

	/* Disable host verification */

	errnum = curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

	if (errnum != CURLE_OK) {
		simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_SSL_VERIFYHOST)", errnum);
		curl_easy_cleanup(curl);
		return -1;
	}

	/* Set certificate */
	if (self_assigned_cert) {
		errnum = curl_easy_setopt(curl, CURLOPT_SSLCERT, simplescim_scim_send_cert.c_str());

		if (errnum != CURLE_OK) {
			simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_SSLCERT)", errnum);
			curl_easy_cleanup(curl);
			return -1;
		}

		/* Set private key */

		errnum = curl_easy_setopt(curl, CURLOPT_SSLKEY, simplescim_scim_send_key.c_str());

		if (errnum != CURLE_OK) {
			simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_SSLKEY)", errnum);
			curl_easy_cleanup(curl);
			return -1;
		}

		/* Set pinned public key */

		errnum = curl_easy_setopt(curl, CURLOPT_PINNEDPUBLICKEY, simplescim_scim_send_pinnedpubkey.c_str());

		if (errnum != CURLE_OK) {
			simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_PINNEDPUBLICKEY)", errnum);
			curl_easy_cleanup(curl);
			return -1;
		}
	}

	/* Set HTTP method */

	errnum = curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, method.c_str());

	if (errnum != CURLE_OK) {
		simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_CUSTOMREQUEST)", errnum);
		curl_easy_cleanup(curl);
		return -1;
	}

	/* Set URL */

	errnum = curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

	if (errnum != CURLE_OK) {
		simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_URL)", errnum);
		curl_easy_cleanup(curl);
		return -1;
	}

	/* Set SCIM resource */

	if (!resource.empty()) {
		errnum = curl_easy_setopt(curl, CURLOPT_POSTFIELDS, resource.c_str());

		if (errnum != CURLE_OK) {
			simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_POSTFIELDS)", errnum);
			curl_easy_cleanup(curl);
			return -1;
		}
	}

	/* Set HTTP headers for SCIM */

	chunk = simplescim_scim_send_create_slist(method);

	if (chunk == nullptr) {
		curl_easy_cleanup(curl);
		return -1;
	}

	errnum = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

	if (errnum != CURLE_OK) {
		simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_HTTPHEADER)", errnum);
		curl_slist_free_all(chunk);
		curl_easy_cleanup(curl);
		return -1;
	}

	/* Set write callback */

	if (response_data != nullptr) {
		errnum = curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, simplescim_scim_send_write_func);

		if (errnum != CURLE_OK) {
			simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_WRITEFUNCTION)", errnum);
			curl_slist_free_all(chunk);
			curl_easy_cleanup(curl);
			return -1;
		}
	}

	/* Set data pointer */

	if (response_data != nullptr) {
		http_response.len = 0;
		http_response.alloc = 1024;
		http_response.data = static_cast<char *>(malloc(http_response.alloc));

		if (http_response.data == nullptr) {
			simplescim_error_string_set_errno("simplescim_scim_send:"
			                                  "malloc");
			curl_slist_free_all(chunk);
			curl_easy_cleanup(curl);
			return -1;
		}

		http_response.data[0] = '\0';

		errnum = curl_easy_setopt(curl, CURLOPT_WRITEDATA, &http_response);

		if (errnum != CURLE_OK) {
			simplescim_scim_send_print_curl_error("curl_easy_setopt(CURLOPT_WRITEDATA)", errnum);
			free(http_response.data);
			curl_slist_free_all(chunk);
			curl_easy_cleanup(curl);
			return -1;
		}
	}

	/* Perform request */

	errnum = curl_easy_perform(curl);

	if (errnum != CURLE_OK) {
		simplescim_scim_send_print_curl_error("curl_easy_perform", errnum);

		if (response_data != nullptr) {
			free(http_response.data);
		}

		curl_slist_free_all(chunk);
		curl_easy_cleanup(curl);
		return -1;
	}

	/* Get response code */

	errnum = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

	if (errnum != CURLE_OK) {
		simplescim_scim_send_print_curl_error("curl_easy_getinfo", errnum);

		if (response_data != nullptr) {
			free(http_response.data);
		}

		curl_slist_free_all(chunk);
		curl_easy_cleanup(curl);
		return -1;
	}

	/* Clean up */

	if (response_data != nullptr) {
		*response_data = http_response.data;
//		std::cout << *response_data << std::endl;
	}

	*response_code = http_code;

	curl_slist_free_all(chunk);
	curl_easy_cleanup(curl);

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
int scim_sender::send_init(std::string cert, std::string key, std::string pinnedpubkey) {
	CURLcode errnum;

	errnum = curl_global_init(CURL_GLOBAL_DEFAULT);

	if (errnum != 0) {
		simplescim_scim_send_print_curl_error("curl_global_init", errnum);
		return -1;
	}

	simplescim_scim_send_cert = std::move(cert);
	simplescim_scim_send_key = std::move(key);
	simplescim_scim_send_pinnedpubkey = std::move(pinnedpubkey);

	return 0;
}

/**
 * Clears simplescim_scim_send and frees any associated
 * dynamically allocated memory.
 */
void scim_sender::send_clear() {
	curl_global_cleanup();
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

	err = simplescim_scim_send(url, body, "POST", &response_data, &response_code);

	if (err == -1) {
		return {};
	}

	std::string result(response_data);
	free(response_data);

	if (response_code != 201) {
		simplescim_error_string_set_prefix("simplescim_scim_send_create");
		std::string message;
		if (response_code == 409) {
			message = " object already exists";
			simplescim_error_string_set_message("HTTP response code %ld returned, expected %ld %s", response_code, 201L,
			                                    message.c_str());
		} if (response_code == 413) {
			message = " data to " + url + " to large.";
			simplescim_error_string_set_message("HTTP response code %ld returned, expected %ld %s", response_code, 201L,
			                                    message.c_str());
			return {};
		}
	}
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
//int scim_sender::send_update(const char *url, const char *resource, const char **response) {
std::optional<std::string>
scim_sender::send_update(const std::string &url, const std::string &body) {
	char *response_data;
	long response_code;
	int err;

	err = simplescim_scim_send(url, body, "PUT", &response_data, &response_code);

	if (err == -1) {
		return {};
	}

	if (response_code != 200) {
		simplescim_error_string_set_prefix("simplescim_scim_send_update");
		simplescim_error_string_set_message("HTTP response code %ld returned, expected %ld", response_code, 200L);
		free(response_data);
		return {};
	}

	return {response_data};
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
int scim_sender::send_delete(const std::string &url) {
	long response_code;
	int err;

	err = simplescim_scim_send(url, "", "DELETE", nullptr, &response_code);

	if (err == -1) {
		return -1;
	}

	if (response_code != 204) {
		simplescim_error_string_set_prefix("simplescim_scim_send_delete");
		simplescim_error_string_set_message("HTTP response code %ld returned, expected %ld", response_code, 204L);
		return response_code;
	}

	return 0;
}
