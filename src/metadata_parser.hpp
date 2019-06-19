#ifndef FEDTLSAUTH_METADATA_PARSER_HPP
#define FEDTLSAUTH_METADATA_PARSER_HPP

#include <string>
#include <vector>
#include <memory>
#include "castore_file.hpp"

namespace federated_tls_auth {

struct pin {
    std::string name;
    std::string value;

    pin(const std::string& n, const std::string& v)
            : name(n), value(v) {}
};

struct server_end_point {
    std::string url;
    std::vector<pin> pins;
    std::shared_ptr<castore_file> castore;
};

std::string concatenate_keys(const std::vector<pin>& pins);

server_end_point load_from_metadata(const std::string& metadata_path,
                                    const std::string& entity_id,
                                    const std::string& server_name);

} // namespace federated_tls_auth

#endif // FEDTLSAUTH_METADATA_PARSER_HPP
