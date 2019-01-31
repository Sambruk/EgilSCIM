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

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>

#include <iostream>

#include "json_data_file.hpp"
#include "config_file.hpp"

//json_data_file::json_data_file(const std::string &fname) : filename(fname) {
//}

//json_data_file::json_data_file() {
//}

void json_data_file::get_users(std::shared_ptr<object_list> list) {
	namespace pt = boost::property_tree;
	pt::ptree root;

	try {
		pt::read_json(filename, root);


		std::string ss12000type = root.get<std::string>("ss12000type", "");
		base_object object(ss12000type);

		string_vector groupName = {root.get<std::string>("groupName", "")};
		std::string id = root.get<std::string>("externalId", "");

		string_vector idList;
		for (pt::ptree::value_type &ids : root.get_child("members")) {
			idList.emplace_back(ids.second.data());
		}
		object.add_attribute("groupName", std::move(groupName));
		object.add_attribute("externalId", {id});
		object.add_attribute("members", std::move(idList));
		list->add_object(id, std::make_shared<base_object>(object));
	} catch (const boost::exception &ex) {
		std::cerr << "Failed to read json file: " << filename << boost::diagnostic_information(ex);
	} catch (...) {
		std::cerr << "Failed to read json file: " << filename << std::endl;
	}
}

std::shared_ptr<object_list> json_data_file::get_users() {
	std::shared_ptr<object_list> list = std::make_shared<object_list>();
	get_users(list);
	return list;
}

relations_vector json_data_file::json_to_ldap_remote_relations(const std::string &json) noexcept {
	if (json.empty())
		return {};
	relations_vector values;

	try {
		std::stringstream json_stream;
		json_stream << json;
		namespace pt = boost::property_tree;
		pt::ptree root;
		pt::read_json(json_stream, root);
		for (pt::ptree::value_type &rels : root.get_child("relations")) {
			relations r;
			r.type = rels.first;
			r.remote_attribute = rels.second.get<std::string>("remote_attribute");

			r.remote_ldap_base = rels.second.get<std::string>("ldap_base");
			r.remote_ldap_filter = rels.second.get<std::string>("ldap_filter");

			r.local_attribute = rels.second.get<std::string>("local_attribute");
			r.method = rels.second.get<std::string>("method");
			values.emplace_back(std::move(r));
		}
	} catch (const boost::exception &ex) {
		std::cerr << "Failed to read json\n" << json << std::endl <<
		          boost::diagnostic_information(ex);
	} catch (...) {
		std::cout << "Failed to read json\n" << json << std::endl;
	}

	return values;
}

data_cache_vector json_data_file::json_to_ldap_cache_requests(const std::string &json) noexcept {
	if (json.empty())
		return {};
	data_cache_vector values;

	try {
		std::stringstream json_stream;
		json_stream << json;
		namespace pt = boost::property_tree;
		pt::ptree root;
		pt::read_json(json_stream, root);
		for (pt::ptree::value_type &rels : root.get_child("caches")) {
			data_cache r;
			r.type = rels.first;
			r.index_attribute = rels.second.get<std::string>("index-attribute");
			r.ldap_base = rels.second.get<std::string>("ldap-base");
			r.filter = rels.second.get<std::string>("ldap-filter");
			values.emplace_back(std::move(r));
		}
	} catch (const boost::exception &ex) {
		std::cerr << "Failed to read json\n" << json << std::endl <<
		          boost::diagnostic_information(ex);
	} catch (...) {
		std::cout << "Failed to read json\n" << json << std::endl;
	}

	return values;
}
std::map<std::string, pair_map> json_data_file::json_cache;

pair_map json_data_file::json_to_ldap_query(const std::string &type) noexcept {
	std::string filter_json = config_file::instance().get(type + "-ldap-filter", true);
	auto cached_json = json_cache.find(type);
	if (cached_json != json_cache.end())
		return cached_json->second;


	pair_map values;

	if (filter_json.empty())
		return values;

	try {
		std::stringstream json_stream;
		json_stream << filter_json;
		namespace pt = boost::property_tree;
		pt::ptree root;
		pt::read_json(json_stream, root);
		for (pt::ptree::value_type &queries : root.get_child("queries")) {
			auto base = queries.second.get<std::string>("base");
			auto filter = queries.second.get<std::string>("ldap");
			values.emplace(queries.first, std::make_pair(base, filter));
		}
	} catch (const boost::exception &ex) {
		std::cerr << "Failed to read json\n" << filter_json << std::endl <<
		          boost::diagnostic_information(ex);
	} catch (...) {
		std::cout << "Failed to read json\n" << filter_json << std::endl;
	}
	json_cache.emplace(std::make_pair(type, values));
	return values;
}

