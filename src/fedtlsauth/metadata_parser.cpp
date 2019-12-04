/*
 * Copyright (c) 2019 Föreningen Sambruk
 *
 * You should have received a copy of the MIT license along with this project.
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "metadata_parser.hpp"
#include <boost/property_tree/json_parser.hpp>
#include <curl/curl.h>

namespace {

#if LIBCURL_VERSION_NUM >= 0x073e00 // 7.62.0

// Properly cleans up a cURL URL handle at end of scope
class CURL_URL {
public:
    CURL_URL(CURLU* h) : guarded(h) {}

    ~CURL_URL() { curl_url_cleanup(guarded); }

    operator CURLU*() const { return guarded; }

private:
    CURLU* guarded;
};

#endif

/* Normalizes a URL so we can compare strings
 * to see if they are logically the same URL.
 *
 * Returns the same string if we're using an old
 * version of cURL without support for URL 
 * normalization.
 */
std::string url_normalize(const std::string& url) {
#if LIBCURL_VERSION_NUM >= 0x073e00 // 7.62.0
    CURL_URL h{curl_url()};
    
    CURLUcode rc = curl_url_set(h, CURLUPART_URL, url.c_str(), CURLU_DEFAULT_SCHEME);
    if (rc != CURLUE_OK) {
        throw std::runtime_error("Bad URL");
    }
    
    char *url_normalized;
    
    if (curl_url_get(h, CURLUPART_URL, &url_normalized, 0) != CURLUE_OK) {
        throw std::runtime_error("Failed to normalize URL");
    }

    std::string result{url_normalized};
    curl_free(url_normalized);

    return result;
#else
    return url;
#endif
}

bool url_equals(const std::string& url1, const std::string& url2) {
    try {
        return url_normalize(url1) == url_normalize(url2);
    }
    catch (const std::runtime_error& e) {
        return false;
    }
}

}

namespace federated_tls_auth {

namespace pt = boost::property_tree;
using std::vector;
using std::string;

// A function type used to match servers either by name or tags
typedef std::function<vector<pt::ptree>(pt::ptree)> server_matcher_fun;

/*
 * Finds servers matching a name.
 *
 * server_name can be empty, in which case we expect to find exactly
 * one server entry in servers.
 *
 * If no matching server is found, or if more than one match,
 * we throw std::runtime_error, 
 */
vector<pt::ptree> server_name_matcher(const string& server_name,
                                      pt::ptree servers) {
    pt::ptree server;
    int server_count = 0;
    for(const auto& cur : servers) {
        ++server_count;
        if ("" == server_name ||
            cur.second.get<string>("name") == server_name) {
            server = cur.second;
        }
    }

    if (server_count > 1 && server_name == "") {
        throw std::runtime_error("Please specify a server name. More than one server matched.");
    }
    else if (server == pt::ptree()) {
        throw std::runtime_error("Failed to find matching server in metadata");
    }
    else {
        vector<pt::ptree> result;
        result.push_back(server);
        return result;
    }
}

string tolower(const string str) {
    string result(str);
    std::transform(str.begin(), str.end(), result.begin(), ::tolower);
    return result;
}

// Converts all tags to lower case, sorts and removes duplicates.
vector<string> to_canonical(const vector<string>& tags) {
    vector<string> result;
    result.resize(tags.size());
    std::transform(tags.begin(), tags.end(), result.begin(), tolower);
    std::sort(result.begin(), result.end());
    auto itr = std::unique(result.begin(), result.end());
    result.resize(std::distance(result.begin(), itr));
    return result;
}

// Gets the tags from a server
vector<string> get_canonical_tags(pt::ptree server) {
    auto itr = server.find("tags");

    if (itr == server.not_found()) {
        return {};
    }

    vector<string> tags;
    for (const auto& tag : itr->second) {
        tags.push_back(tag.second.get<string>(""));
    }

    return to_canonical(tags);
}

/*
 * Finds servers matching a set of tags.
 *
 * Throws std::runtime_error if no matching servers are found.
 */
vector<pt::ptree> server_tags_matcher(const vector<string>& tags,
                                      pt::ptree servers) {
    auto canonical_tags = to_canonical(tags);
    vector<pt::ptree> result;
        
    for(const auto& cur : servers) {
        auto current_tags = get_canonical_tags(cur.second);
        if (canonical_tags == current_tags) {
            result.push_back(cur.second);
        }
    }

    if (result.empty()) {
        throw std::runtime_error("Failed to find matching server in metadata");
    }
    
    return result;
}

server_connection_info get_server(const string& metadata_path,
                                  const string& entity_id,
                                  server_matcher_fun server_matcher) {
    server_connection_info result;
    pt::ptree root;
    pt::read_json(metadata_path, root);

    // Find the correct entity
    auto itr = root.find("entities");

    if (itr == root.not_found()) {
        throw std::runtime_error("Didn't find entities in metadata");
    }

    pt::ptree entity;
    for (const auto& cur : itr->second) {
        if (url_equals(cur.second.get<string>("entity_id"), entity_id)) {
            entity = cur.second;
        }
    }

    if (entity == pt::ptree()) {
        throw std::runtime_error("Failed to find matching entity in metadata");
    }

    // Find the correct server (make sure there's only one if server_name is empty)
    itr = entity.find("servers");

    if (itr == entity.not_found()) {
        throw std::runtime_error("Didn't find any servers for given entity in metadata");
    }

    auto servers = server_matcher(itr->second);

    for (const auto& server : servers) {
        server_end_point end_point;
        // Set URL
        end_point.url = server.get<string>("base_uri");

        // Set pinned public keys (possibly multiple keys)
        auto itr = server.find("pins");

        if (itr == entity.not_found()) {
            throw std::runtime_error("Didn't find any pins for server in metadata");
        }

        for (const auto& cur : itr->second) {
            auto name = cur.second.get<string>("name");
            auto value = cur.second.get<string>("value");

            end_point.pins.push_back(pin(name, value));
        }

        if (end_point.pins.empty()) {
            throw std::runtime_error("Didn't find any pins for server in metadata");
        }
        
        result.end_points.push_back(end_point);
    }
        
    // Get the certificates
    itr = entity.find("issuers");

    if (itr == entity.not_found()) {
        throw std::runtime_error("Didn't find any certificates for entity in metadata");
    }

    string all_certs = "";
    for (const auto& cur : itr->second) {
        auto cert = cur.second.get<string>("x509certificate");
        all_certs += cert + "\n";
    }

    if (all_certs.empty()) {
        throw std::runtime_error("Didn't find any certificates for entity in metadata");
    }

    // Place the certificates in a temporary file
    result.castore.reset(new castore_file());
    result.castore->write(all_certs.c_str(), all_certs.size());

    return result;
}

server_connection_info get_server_by_name(const string& metadata_path,
                                          const string& entity_id,
                                          const string& server_name) {
    return get_server(metadata_path, entity_id,
                      [server_name](pt::ptree servers) {
                          return server_name_matcher(server_name, servers);
                      });
}

server_connection_info get_server_by_tags(const string& metadata_path,
                                          const string& entity_id,
                                          const vector<string>& tags) {
    return get_server(metadata_path, entity_id,
                      [tags](pt::ptree servers) {
                          return server_tags_matcher(tags, servers);
                      });
}

string concatenate_keys(const vector<pin>& pins) {
    string result = "";
    for (const auto& cur : pins) {
        if (!result.empty()) {
            result += ";";
        }
        result += cur.name + "//" + cur.value;
    }
    return result;
}

}
