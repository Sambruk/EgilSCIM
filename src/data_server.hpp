//
// Created by Ola Mattsson on 2018-09-20.
//

#ifndef EGILSCIMCLIENT_DATA_SERVER_HPP
#define EGILSCIMCLIENT_DATA_SERVER_HPP


#include <memory>
#include <set>
#include "model/object_list.hpp"

class data_server {
	// static data is loaded once. They are known full sets like SchoolUnit
	// All are loaded initially.
	// Also generated object like Employment and Activity are static.
	std::map<std::string, std::shared_ptr<object_list>> static_data;
	// dynamic data is objects like User, they are loaded as a consequence of loading
	// groups and generated data.
	// The distiction is if we can say "it is loaded or not" v.s. it grows as we load
	// other data
	std::map<std::string, std::shared_ptr<object_list>> dynamic_data;
	string_vector static_types;
	string_vector dynamic_types;

	data_server(const data_server &other) = default;

	data_server() = default;


public:
	static data_server &instance() {
		static data_server s;
		return s;
	}

	void clear() {
		static_types.clear();
		dynamic_types.clear();
		static_data.clear();
		dynamic_data.clear();
	}

	std::shared_ptr<base_object>
	find_object_by_attribute(const std::string &type, const std::string &attrib, const std::string &value);
	std::shared_ptr<object_list> get_by_type(const std::string &type);

	/**
	 * dependent data that is loaded in its entirety once
	 * @param type
	 * @return
	 */
	std::shared_ptr<object_list> get_static_by_type(const std::string &type);


	std::shared_ptr<object_list> getAllObjects();

	void load();

	void preload();

	void add(const std::string &type, object_list &&list);

	void add(const std::string &type, const object_list &list);

	void add(const std::string &type, base_object &&object);

private:
	/**
	* static_data is loaded once, this will assert if called with existing type
	* @param type
	* @param list
	*/
	void add_static(const std::string &type, object_list &&list);

	void add_static(const std::string &type, const object_list &list);

	/**
	 * add dynamic data, this is appended to as static data is load
	 * @param type
	 * @param list
	 */
	void add_dynamic(const std::string &type, object_list &&list);

	void add_dynamic(const std::string &type, const object_list &list);

};


#endif //EGILSCIMCLIENT_DATA_SERVER_HPP
