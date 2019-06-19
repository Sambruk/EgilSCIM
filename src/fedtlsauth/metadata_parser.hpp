#ifndef FEDTLSAUTH_METADATA_PARSER_HPP
#define FEDTLSAUTH_METADATA_PARSER_HPP

#include <string>
#include <vector>
#include <memory>
#include "castore_file.hpp"

namespace federated_tls_auth {

// A pinned public key
struct pin {
    // The hash method, typically sha256
    std::string name;

    // Base64 encoded hash of key
    std::string value;

    pin(const std::string& n, const std::string& v)
            : name(n), value(v) {}
};

// A server_end_point contains all information
// the client needs in order to connect to a server,
// authenticate it and start sending requests to its API.
struct server_end_point {
    // The base URL for the end-point
    std::string url;

    // One or several pinned public keys
    std::vector<pin> pins;

    /*
     * A temporary file containing certificates for the server.
     *
     * The temporary file is deleted when the castore_file object
     * is deleted, so keep this object around until the authentication
     * has been performed.
     */
    std::shared_ptr<castore_file> castore;
};

/*
 * Concatenates (possibly) multiple pinned public keys to one string in a 
 * format recognized by e.g. curl.
 */
std::string concatenate_keys(const std::vector<pin>& pins);

/*
 * Gets connection and authentication information for an entity and server
 * from a metadata file.
 *
 * server_name can be empty if there is only one server for the entity.
 *
 * Throws a std::runtime_error if the file is malformed, or if the entity
 * or server is missing.
 */
server_end_point load_from_metadata(const std::string& metadata_path,
                                    const std::string& entity_id,
                                    const std::string& server_name);

} // namespace federated_tls_auth

#endif // FEDTLSAUTH_METADATA_PARSER_HPP
