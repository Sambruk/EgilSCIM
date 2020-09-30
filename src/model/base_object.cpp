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

#include <map>
#include <memory>
#include <iostream>

#include "base_object.hpp"
#include "../utility/simplescim_error_string.hpp"
#include "../config_file.hpp"

std::string base_object::get_uid(bool search) const {
	if (identity.empty() && search) {

		/** Get LDAP attribute that is unique identifier */
		config_file &conf = config_file::instance();
		std::string uid_attr;
		std::string type = getSS12000type();
		if (type == "base")
			uid_attr = conf.require("User-unique-identifier");
		else {
			uid_attr = conf.require(type + "-unique-identifier");
			auto pos = uid_attr.find(',');
			if (pos != std::string::npos)
				uid_attr.erase(pos);
		}
		ss12000type = type;

		if (uid_attr.empty()) {
			return "";
		}
		std::string::size_type dotPos = uid_attr.find('.');
		if (dotPos != std::string::npos)
			uid_attr = uid_attr.substr(dotPos + 1);
		/* Get unique identifier value */
		string_vector values = get_values(uid_attr);
		if (values.empty()) {
			values = get_values(type + '.' + uid_attr);
		}
		// still empty?
		if (values.empty()) {
			std::string cn;
			const string_vector &l = get_values("cn");
			if (!l.empty())
				cn = l.at(0);
			std::cerr
					<< "base_object::get_uid "
					<< cn << " missing attribute " << uid_attr
					<< " for type [" << type << "]"
					<< std::endl;
			return "";
		}

		if (values.size() != 1) {
			simplescim_error_string_set_prefix("simplescim_user_get_uid");
			simplescim_error_string_set_message("attribute \"%s\" must have exactly one value", uid_attr.c_str());
			return "";
		}

		/* Allocate unique identifier copy */
		identity = values.at(0);
	}
	return identity;
}

std::ostream &operator<<(std::ostream &os, const base_object &object) {
	static std::string quote("\"");
	static std::string tab("\t");

	os << "{";

	for (const auto &attributes: object.attributes) {
		for (const auto &value : attributes.second) {
			os << tab << quote << attributes.first << quote << "\" : \"" << value << quote << ",\n";

		}
	}

	os << "}";
	return os;
}

std::shared_ptr<base_object> vector_to_base_object(const string_vector& values,
                                                      const string_vector& attribute_names,
                                                      const std::string& type) {
    attrib_map attributes;

    for (size_t i = 0; i < values.size(); ++i) {
        attributes[attribute_names[i]] = string_vector({values[i]});
    }

    attributes["ss12000type"] = string_vector({type});
    return std::make_shared<base_object>(std::move(attributes));
}