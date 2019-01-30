/**
 * Created by Ola Mattsson.
 *
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
 *
 * Further development with groups and relations support
 * by Ola Mattsson - IT informa for Sambruk
 */

#ifndef SIMPLESCIM_SCIM_DATA_HPP
#define SIMPLESCIM_SCIM_DATA_HPP


#include <memory>
#include "model/object_list.hpp"
#include "config_file.hpp"

class scim_data {
	const std::shared_ptr<object_list> list;

	std::vector<std::string> findTypes();

public:
	explicit scim_data(std::shared_ptr<object_list> theData) : list(theData) {}

	void findRelations();
//	std::shared_ptr<object_list> generateRelations(const std::string &scim_type);

	std::vector<std::string> variblesAsVector(const std::string &s);
};


#endif //SIMPLESCIM_SCIM_DATA_HPP
