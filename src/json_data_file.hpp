//
// Created by Ola Mattsson on 2018-08-31.
//

#ifndef SIMPLESCIM_JSON_DATA_FILE_HPP
#define SIMPLESCIM_JSON_DATA_FILE_HPP


#include <sstream>
#include "model/object_list.hpp"

struct relations {
	std::string type;
	std::string remote_attribute;
	std::string local_attibute;
	std::string method;
};
struct data_cache {
	std::string type;
	std::string index_attribute;
	std::string ldap_base;
	std::string filter;

};
typedef std::map<std::string, std::pair<std::string, std::string>> pair_map;
typedef std::vector<relations> relations_vector;
typedef std::vector<data_cache> data_cache_vector;
class json_data_file {
	std::string filename;
	std::stringstream json;

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
