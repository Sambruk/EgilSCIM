/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2025 Föreningen Sambruk
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

#ifndef EGILSCIMCLIENT_GENERATED_ORGANISATION_LOAD_HPP
#define EGILSCIMCLIENT_GENERATED_ORGANISATION_LOAD_HPP

#include <memory>
#include "model/object_list.hpp"
#include "utility/indented_logger.hpp"

/// Generates an Organisation object from a static UUID and displayName
/** The UUID and displayName is configured in the configuration file so
 *  the organisation object doesn't need to exist in the data source.
 */
std::shared_ptr<object_list> get_generated_organisation(const std::string& type,
    indented_logger& load_logger);

/** Configures some default configuration variables for the Organisation type 
 *  Since the Organisation object typically works the same way in every
 *  installation, we can make the configuration a bit more convenient
 *  by supplying a default JSON template e.g.
 */
void setup_default_organisation_config_variables();

#endif // EGILSCIMCLIENT_GENERATED_ORGANISATION_LOAD_HPP
