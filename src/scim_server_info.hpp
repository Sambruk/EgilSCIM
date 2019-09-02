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

#ifndef EGILSCIM_SCIM_SERVER_INFO_HPP
#define EGILSCIM_SCIM_SERVER_INFO_HPP

#include <string>
#include <memory>
#include "fedtlsauth/castore_file.hpp"

class config_file;

/*
 * The SCIMServerInfo class keeps track of the parameters
 * we need in order to connect to a SCIM server.
 *
 * This class is responsible for getting these parameters,
 * either from the config file or from the metadata file.
 */
class SCIMServerInfo {
public:
    SCIMServerInfo(const config_file& config);
    ~SCIMServerInfo();

    /*
     * The base URL for the SCIM end-point.
     * For instance https://example.com/scim/v2
     */
    std::string get_url() const;

    /*
     * One or more pinned public keys, in format
     * as expected by CURL's CURLOPT_PINNEDPUBLICKEY option.
     */
    std::string get_pinned_public_keys() const;

    // Full path to CA bundle file
    std::string get_ca_bundle_path() const;

private:    
    std::string url;
    std::string pinned_public_keys;
    std::string ca_bundle_path;

    std::shared_ptr<federated_tls_auth::castore_file> castore_file;
};

#endif // EGILSCIM_SCIM_SERVER_INFO_HPP
