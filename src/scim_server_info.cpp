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

#include "scim_server_info.hpp"
#include "config_file.hpp"
#include "fedtlsauth/metadata_parser.hpp"
#include <experimental/filesystem>

using namespace std::experimental;

SCIMServerInfo::SCIMServerInfo(const config_file& config) {
    if (config.has("metadata-path")) {
        auto metadata_path = config.get_path("metadata-path");
        auto entity_id = config.get("metadata-entity");
        auto server_name = config.get("metadata-server", true);

        try {
            auto end_point =
                federated_tls_auth::load_from_metadata(metadata_path, entity_id, server_name);

            url = end_point.url;
            pinned_public_keys = federated_tls_auth::concatenate_keys(end_point.pins);
            ca_bundle_path = end_point.castore->get_path();
            castore_file = end_point.castore;
        }
        catch (const std::runtime_error& e) {
            std::cerr << "Failed to load metadata from " << metadata_path << std::endl;
            throw;
        }
    }
    else {
        url = config.get("scim-url");
        pinned_public_keys = config.get("pinnedpubkey");
        ca_bundle_path = filesystem::path(config.get_path("metadata_ca_path")) / config.get("metadata_ca_store");
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
