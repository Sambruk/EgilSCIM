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

#include "simpleurl.hpp"
#include <curl/curl.h>
#include <memory>
#include <stdexcept>

namespace {
size_t write_http_response(void *buffer, size_t size, size_t nmemb,
                        void *destination) {
    std::vector<char> *http_response = static_cast<std::vector<char>*>(destination);
    char *chbuf = static_cast<char*>(buffer);
    size_t len = size * nmemb;
    std::copy(chbuf, chbuf+len, std::back_inserter(*http_response));
    return len;
}
}

namespace simpleurl {
int read(const std::string& url, std::vector<char>& destination) {
    destination.clear();

    CURL* curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        std::shared_ptr<int> cleanUpCURL(NULL, [&](int *) { curl_easy_cleanup(curl); });

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        /* Follow redirection */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        /* Define our callback to get called when there is data to be written */
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_http_response);
        /* Set a pointer to our struct to pass to the callback */
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &destination);

        /* Perform the request */
        res = curl_easy_perform(curl);

        if (res != CURLE_OK) {
            throw std::runtime_error("failed to fetch " + url + " : " + curl_easy_strerror(res));
        }

        /* Get response code */
        long http_code;
        res = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

        if (res != CURLE_OK) {
            throw std::runtime_error("failed to get response code for " + url + " : " + curl_easy_strerror(res));
        }

        return static_cast<int>(http_code);
    }
    else {
        throw std::runtime_error("failed to initialize curl");
    }
}
}
