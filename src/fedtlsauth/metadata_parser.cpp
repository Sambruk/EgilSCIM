#include "metadata_parser.hpp"
#include <boost/property_tree/json_parser.hpp>

namespace federated_tls_auth {

server_end_point load_from_metadata(const std::string& metadata_path,
                                    const std::string& entity_id,
                                    const std::string& server_name) {
    server_end_point result;
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
    result.url = server.get<std::string>("base_uri");

    // Set pinned public keys (possibly multiple keys)
    itr = server.find("pins");

    if (itr == entity.not_found()) {
        throw std::runtime_error("Didn't find any pins for server in metadata");
    }

    for (const auto& cur : itr->second) {
        auto name = cur.second.get<std::string>("name");
        auto value = cur.second.get<std::string>("value");

        result.pins.push_back(pin(name, value));
    }

    if (result.pins.empty()) {
        throw std::runtime_error("Didn't find any pins for server in metadata");
    }
        
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
    result.castore.reset(new castore_file());
    result.castore->write(all_certs.c_str(), all_certs.size());

    return result;
}

std::string concatenate_keys(const std::vector<pin>& pins) {
    std::string result = "";
    for (const auto& cur : pins) {
        if (!result.empty()) {
            result += ";";
        }
        result += cur.name + "//" + cur.value;
    }
    return result;
}
    
}
