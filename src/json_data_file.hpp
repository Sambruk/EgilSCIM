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

#ifndef SIMPLESCIM_JSON_DATA_FILE_HPP
#define SIMPLESCIM_JSON_DATA_FILE_HPP


#include <sstream>
#include "model/object_list.hpp"

struct relations {
    std::string type;
    std::string remote_attribute;
    std::string remote_ldap_base;
    std::string remote_ldap_filter;
    std::string local_attribute;
    std::string method;

    /*
     * Returns LDAP base and filter from the relation.
     * Any occurence of ${value} in the base or filter will be
     * replaced by whatever is in the variable value.
     */
    std::pair<std::string, std::string> get_ldap_filter(const std::string &value);
};

struct data_cache {
	std::string type;
	std::string index_attribute;
	std::string ldap_base;
	std::string filter;

};
using pair_map = std::map<std::string, std::pair<std::string, std::string>>;
using relations_vector = std::vector<relations>;
using data_cache_vector = std::vector<data_cache> ;

class json_data_file {
	std::string filename;
	std::stringstream json;
	static std::map<std::string, pair_map> json_cache;

public:

	std::shared_ptr<object_list> get_users();

	void get_users(std::shared_ptr<object_list> list);

	static pair_map
	json_to_ldap_query(const std::string &json) noexcept;

	static data_cache_vector
	json_to_ldap_cache_requests(const std::string &json) noexcept;

	static relations_vector
	json_to_ldap_remote_relations(const std::string &json) noexcept;

};


#endif //SIMPLESCIM_JSON_DATA_FILE_HPP
