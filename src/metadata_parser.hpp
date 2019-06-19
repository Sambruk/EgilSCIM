#ifndef FEDTLSAUTH_METADATA_PARSER_HPP
#define FEDTLSAUTH_METADATA_PARSER_HPP

#include <string>
#include <vector>
#include <memory>
#include "tempfile.hpp"

namespace FederatedTLSAuth {

struct Pin {
    std::string name;
    std::string value;

    Pin(const std::string& n, const std::string& v)
            : name(n), value(v) {}
};

struct ServerEndPoint {
    std::string url;
    std::vector<Pin> pins;
    std::shared_ptr<Tempfile> ca_store;
};

std::string concatenate_keys(const std::vector<Pin>& pins);

ServerEndPoint load_from_metadata(const std::string& metadata_path,
                                  const std::string& entity_id,
                                  const std::string& server_name);

}

#endif // FEDTLSAUTH_METADATA_PARSER_HPP
