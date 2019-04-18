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

SCIMServerInfo::SCIMServerInfo(const config_file& config) {
    url = config.get("scim-url");
    pinned_public_keys = config.get("pinnedpubkey");
    ca_bundle_path = config.get("metadata_ca_path") + config.get("metadata_ca_store");
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
