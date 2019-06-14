/**
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
 */

#include "scim_server_info.hpp"
#include "config_file.hpp"
#include <boost/property_tree/json_parser.hpp>

SCIMServerInfo::SCIMServerInfo(const config_file& config) {
    if (config.has("metadata-path")) {
        auto metadata_path = config.get("metadata-path");
        auto entity_id = config.get("metadata-entity");
        auto server_name = config.get("metadata-server", true);

        load_from_metadata(metadata_path, entity_id, server_name);
    }
    else {
        url = config.get("scim-url");
        pinned_public_keys = config.get("pinnedpubkey");
        ca_bundle_path = config.get("metadata_ca_path") + config.get("metadata_ca_store");
    }
}

SCIMServerInfo::~SCIMServerInfo() {
}

std::string SCIMServerInfo::get_url() const {
    return url;
}

std::string SCIMServerInfo::get_pinned_public_keys() const {
    return pinned_public_keys;
}

std::string SCIMServerInfo::get_ca_bundle_path() const {
    return ca_bundle_path;
}

void SCIMServerInfo::load_from_metadata(const std::string& metadata_path,
                                        const std::string& entity_id,
                                        const std::string& server_name) {

    try {
        boost::property_tree::ptree root;
        boost::property_tree::read_json(metadata_path, root);

        // Find the correct entity
        auto itr = root.find("entities");

        if (itr == root.not_found()) {
            throw std::runtime_error("Didn't find entities in metadata");
        }

        boost::property_tree::ptree entity;
        for (const auto& cur : itr->second) {
            if (cur.second.get<std::string>("entity_id") == entity_id) {
                entity = cur.second;
            }
        }

        if (entity == boost::property_tree::ptree()) {
            throw std::runtime_error("Failed to find matching entity in metadata");
        }

        // Find the correct server (make sure there's only one if server_name is empty)
        itr = entity.find("servers");

        if (itr == entity.not_found()) {
            throw std::runtime_error("Didn't find any servers for given entity in metadata");
        }

        boost::property_tree::ptree server;
        int server_count = 0;
        for(const auto& cur : itr->second) {
            ++server_count;
            if ("" == server_name ||
                cur.second.get<std::string>("name") == server_name) {
                server = cur.second;
            }
        }

        if (server_count > 1 && server_name == "") {
            throw std::runtime_error(std::string("Please specify a server name. Entity ") + entity_id + " has more than one server.");
        }
        else if (server == boost::property_tree::ptree()) {
            throw std::runtime_error("Failed to find matching server in metadata");
        }

        // Set URL
        url = server.get<std::string>("base_uri");

        // Set pinned public keys (possibly multiple keys)
        itr = server.find("pins");

        if (itr == entity.not_found()) {
            throw std::runtime_error("Didn't find any pins for server in metadata");
        }

        std::string pins = "";
        for (const auto& cur : itr->second) {
            auto name = cur.second.get<std::string>("name");
            auto value = cur.second.get<std::string>("value");

            if (!pins.empty()) {
                pins += ";";
            }
            pins += name + "//" + value;
        }

        if (pins.empty()) {
            throw std::runtime_error("Didn't find any pins for server in metadata");
        }

        pinned_public_keys = pins;

        // Get the certificates
        itr = entity.find("issuers");

        if (itr == entity.not_found()) {
            throw std::runtime_error("Didn't find any certificates for entity in metadata");
        }

        std::string all_certs = "";
        for (const auto& cur : itr->second) {
            auto cert = cur.second.get<std::string>("x509certificate");
            all_certs += cert + "\n";
        }

        if (all_certs.empty()) {
            throw std::runtime_error("Didn't find any certificates for entity in metadata");
        }

        // Place the certificates in a temporary file
        tempfile.reset(new Tempfile());
        tempfile->write(all_certs.c_str(), all_certs.size());

        ca_bundle_path = tempfile->get_path();
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Failed to load metadata from " << metadata_path << std::endl;
        throw;
    }
}
