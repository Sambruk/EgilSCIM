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
        auto tags = config.get_vector("metadata-tags", true);

        if (tags.empty()) {
            tags = {"egilv1"};
        }

        try {
            auto connection_info =
                server_name.empty() ?
                federated_tls_auth::get_server_by_tags(metadata_path, entity_id, tags) :
                federated_tls_auth::get_server_by_name(metadata_path, entity_id, server_name);

            auto end_point = connection_info.end_points.front();
            url = end_point.url;
            pinned_public_keys = federated_tls_auth::concatenate_keys(end_point.pins);
            ca_bundle_path = connection_info.castore->get_path();
            castore_file = connection_info.castore;
        }
        catch (const std::runtime_error&) {
            std::cerr << "Failed to load metadata from " << metadata_path << std::endl;
            throw;
        }
    }
    else {
        url = config.get("scim-url");
        pinned_public_keys = config.get("pinnedpubkey");
        ca_bundle_path = (filesystem::path(config.get_path("metadata_ca_path")) / filesystem::path(config.get("metadata_ca_store"))).u8string();
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
