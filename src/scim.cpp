/**
 * Copyright © 2017-2018  Max Wällstedt <>
 *
 * This file is part of SimpleSCIM.
 *
 * SimpleSCIM is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * SimpleSCIM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with SimpleSCIM.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <algorithm>

#include "scim.hpp"
#include "utility/simplescim_error_string.hpp"
#include "utility/utils.hpp"
#include "model/base_object.hpp"
#include "model/object_list.hpp"
#include "config_file.hpp"
#include "cache_file.hpp"
#include "scim_json_parse.hpp"
#include "simplescim_scim_send.hpp"
#include "local_id_store.hpp"


variables::variables() {
	const config_file &config = config_file::instance();
	variable_entries.emplace(std::make_pair("cert", config.require("cert")));
	variable_entries.emplace(std::make_pair("key", config.require("key")));
	variable_entries.emplace(std::make_pair("pinnedpubkey", config.require("pinnedpubkey")));
//	variable_entries.emplace(std::make_pair("scim-url", config.require("scim-url")));
//	variable_entries.emplace(std::make_pair("scim-resource-identifier", config.require("scim-resource-identifier")));
//	variable_entries.emplace(std::make_pair("scim-create", config.require("scim-create")));
//	variable_entries.emplace(std::make_pair("scim-update", config.require("scim-update")));
//	variable_entries.emplace(
//			std::make_pair("user-scim-resource-identifier", config.require("user-scim-resource-identifier")));
}


int ScimActions::simplescim_scim_init() const {
	int err;

	/* Fetch variables from configuration file */

	if (!vars.valid()) {
		return -1;
	}

	/* Allocate new cache */

	if (scim_new_cache == nullptr) {
		return -1;
	}

	/* Initialise simplescim_scim_send */
	err = scim_sender::instance().send_init(vars.get("cert"), vars.get("key"), vars.get("pinnedpubkey"));

	if (err == -1) {
		return -1;
	}

	return 0;
}

void ScimActions::simplescim_scim_clear() const {
	/* Clear simplescim_scim_send */
	scim_sender::instance().send_clear();

	/* Delete new cache */
	scim_new_cache->clear();
}

/**
 * Makes SCIM requests by comparing the two user lists and
 * reading JSON templates from the configuration file.
 * Updates (or creates) the cache file.
 * On success, zero is returned. On error, -1 is returned
 * and simplescim_error_string is set to an appropriate
 * error message.
 */
int ScimActions::perform(const data_server &current, const object_list &cached) const {
	int err;

	/* Initialise SCIM */
	err = simplescim_scim_init();

	if (err == -1) {
		return -1;
	}
	std::string types_string = config_file::instance().get("scim-type-send-order");
	string_vector types = string_to_vector(types_string);
	for (auto &&type : types) {
		std::shared_ptr<object_list> allOfType = current.get_by_type(type);
		if (!allOfType) {
//			std::cerr << "cant send " << type << ", missing" << std::endl;
			allOfType = std::make_shared<object_list>();
		}
		err = allOfType->process_changes(cached, *this, type);
		if (err != 0) {
			std::cerr << "failed to send " << type << std::endl;
			return -1;
		}
	}

	/* Save new cache file */
	err = cache_file::instance().save(scim_new_cache);

	/* Clean up */
	simplescim_scim_clear();

	return err;
}

int ScimActions::copy_func::operator()(const ScimActions &actions) {
	if (cached.get_uid().empty()) {
		return -1;
	}

	actions.scim_new_cache->add_object(cached.get_uid(),
	                                   std::make_shared<base_object>(cached));

	return 0;

}

int ScimActions::delete_func::operator()(const ScimActions &actions) {

	if (object.get_uid().empty()) {
		simplescim_error_string_set_prefix("ScimActions::delete_func:"
		                                   "get-attribute");
		simplescim_error_string_set_message("cached user does not have attribute \"%s\"",
		                                    actions.vars.get("user-scim-resource-identifier").c_str());
		return -1;
	}
	std::string url = config_file::instance().get("scim-url");
	std::string urlified = unifyurl(object.get_uid());
	std::string endpoint = config_file::instance().get(object.getSS12000type() + "-scim-url-endpoint");
	url += '/' + endpoint + '/' + urlified;
	/* Send SCIM delete request */
	int err = scim_sender::instance().send_delete(url);

	if (err != 0 && err != 404) { // Recache if delete failed, 404 is no failure
		actions.scim_new_cache->add_object(object.get_uid(), std::make_shared<base_object>(object));
	}

	return err;
}

int ScimActions::create_func::operator()(const ScimActions &actions) {
//	struct json_object *scim_id_obj;
	struct json_object *jobj;
	enum json_tokener_error jerr;

	base_object copied_user(create);
	/* Create JSON object for object */
	std::string type = copied_user.getSS12000type();
	if (type == "base") {
		type = "User";
	}
	std::string create_var = type + "-scim-json-template";
	std::string template_json = actions.conf.get(create_var);
	std::string parsed_json = scim_json_parse(template_json, copied_user);

	if (parsed_json.empty()) {
		std::cerr << "ScimActions::create_func: Failed to parse json" << std::endl;
		return -1;
	}

	/* Verify JSON string */
	jobj = json_tokener_parse_verbose(parsed_json.c_str(), &jerr);
	if (jobj == nullptr) {
		std::cout << parsed_json << std::endl;
		simplescim_error_string_set_prefix("create_object_func:"
		                                   "json_tokener_parse_verbose");
		simplescim_error_string_set_message("%s", json_tokener_error_desc(jerr));
		return -1;
	}

	std::string url = config_file::instance().get("scim-url");
	std::string endpoint = config_file::instance().get(type + "-scim-url-endpoint");
	url += '/' + endpoint;

//	const string_vector &v = create.get_values("GUID");
//	std::string id;
//	if (v.size() == 1)
//		id = v.at(0);
//	std::cout << "Create: " << url << "( " << id << " )"<< std::endl;

	/* Send SCIM create request */
	std::optional<std::string>
			response_json = scim_sender::instance().send_create(url,
			                                                    json_object_to_json_string_ext(jobj,
			                                                                                   JSON_C_TO_STRING_SPACED |
			                                                                                   JSON_C_TO_STRING_PRETTY)
//	                                                                               ,
//	                                          (const char **) &response_json
	);
	if (!response_json) {
		json_object_put(jobj);
		return -1;
	}

	json_object_put(jobj);

	/* Get SCIM resource identifier */
	jobj = json_tokener_parse_verbose(response_json->c_str(), &jerr);

	if (jobj == nullptr) {
		std::cout << parsed_json << std::endl;
		simplescim_error_string_set_prefix("create_object_func:"
		                                   "json_tokener_parse_verbose");
		simplescim_error_string_set_message("%s", json_tokener_error_desc(jerr));
		return -1;
	}


//	if (!json_object_object_get_ex(jobj, actions.vars.get("scim-resource-identifier").c_str(), &scim_id_obj)) {
//		simplescim_error_string_set_prefix("create_object_func:"
//		                                   "json_object_object_get_ex");
//		simplescim_error_string_set_message("variable \"%s\" is not present in JSON response",
//		                                    actions.vars.get("scim-resource-identifier").c_str());
//		json_object_put(jobj);
//		return -1;
//	}

//	if (!json_object_is_type(scim_id_obj, json_type_string)) {
//		simplescim_error_string_set_prefix("create_object_func:"
//		                                   "json_object_is_type");
//		simplescim_error_string_set_message("variable \"%s\" is not a string in JSON response",
//		                                    actions.vars.get("scim-resource-identifier").c_str());
//		json_object_put(jobj);
//		return -1;
//	}

//	const char *scim_id_str = json_object_get_string(scim_id_obj);

	/* Copy object */
//	base_object copied_user(create);

	std::string uid = copied_user.get_uid();

	if (uid.empty()) {
		json_object_put(jobj);
		return -1;
	}

	actions.scim_new_cache->add_object(uid, std::make_shared<base_object>(copied_user));


	/* Clean up */
	json_object_put(jobj);

	return 0;
}

int ScimActions::update_func::operator()(const ScimActions &actions) {
//	char *response_json;
	struct json_object *jobj;
	enum json_tokener_error jerr;
//	int err;

	/* Copy object */
	base_object copied_user(object);

	std::string uid = copied_user.get_uid();

	if (uid.empty()) {
		return -1;
	}

	/* Create JSON object for object */
	std::string type = object.getSS12000type();
	if (type == "base") {
		type = "User";
	}
	std::string create_var = type + "-scim-json-template";
	std::string template_json = config_file::instance().get(create_var);
	std::string parsed_json = scim_json_parse(template_json, copied_user);

	if (parsed_json.empty()) {
		return -1;
	}

	/* Verify JSON string */
	jobj = json_tokener_parse_verbose(parsed_json.c_str(), &jerr);

	if (jobj == nullptr) {
		std::cout << parsed_json << std::endl;
		simplescim_error_string_set_prefix("update_object_func:"
		                                   "json_tokener_parse_verbose");
		simplescim_error_string_set_message("%s", json_tokener_error_desc(jerr));
		return -1;
	}

	/**
	 * Send SCIM update request
	 * todo !! use externalid/whatever uuid we produce
	 * */


	std::string unified = unifyurl(object.get_uid());
	std::string url = config_file::instance().get("scim-url");
	std::string endpoint = config_file::instance().get(type + "-scim-url-endpoint");
	url += '/' + endpoint; //+ '/' + unified;

//	std::cout << "Update: " << url << std::endl;
	std::optional<std::string>
			response_json = scim_sender::instance().send_update(
			url, json_object_to_json_string_ext(jobj, JSON_C_TO_STRING_SPACED |
			                                          JSON_C_TO_STRING_PRETTY)
//	                                                                                      		,
//	                                          (const char **) &response_json
	);

	/* Insert copied object into new cache */
	if (!response_json) {
		actions.scim_new_cache->add_object(uid, std::make_shared<base_object>(object));
		json_object_put(jobj);
		return -1;
	} else {
		actions.scim_new_cache->add_object(uid, std::make_shared<base_object>(copied_user));
	}

//	free(response_json);
	json_object_put(jobj);


	return 0;

}
