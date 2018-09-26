//
// Created by Ola Mattsson on 2018-09-01.
//

#include <iostream>
#include <memory>
#include <set>
#include "utility/utils.hpp"
#include "scim_data.hpp"


/**
 * skapa studentgroup - title
 * employment = mellan-db
 * activity = mellan-db
 */

/**
 * object_list - the list of users loaded from db or cache
 * 		map of uid and base_objects
 * 		std::map<std::string, std::shared_ptr<base_object>> objects;
 *
 * base_object
 * 		map of attributes of attribute namd and a vector of strings/values
 * 		std::map<std::string, std::vector<std::string>> attributes{};
 *
 */
std::vector<std::string> findSourceTypes(const std::string &vars) {
	std::set<std::string> result;
	std::string::const_iterator iter = vars.begin();
	std::string::const_iterator end = vars.end();

	while (iter != end) {
		std::string::size_type dot_pos = std::string(iter, end).find('.');
		if (dot_pos != std::string::npos) {
			std::string source_type(iter, iter + dot_pos);
			result.insert(source_type);

			std::string::size_type nextCommaPos = std::string(iter, end).find_first_of(',');
			if (nextCommaPos == std::string::npos) {
				iter = end;
			} else {
				iter += nextCommaPos;
				iter += std::string(iter, end).find_first_not_of(", ");
			}
		} else
			iter = end;
	}
	std::vector<std::string> types;
	for (auto &&item : result) {
		types.emplace_back(item);
	}
	return types;
}

class TypeVariable {
	std::string type;
	std::string variable;
public:
	explicit TypeVariable(const std::string &s) {
		std::string::size_type dotPos = s.find('.');
		if (dotPos != std::string::npos) {
			type = s.substr(0, dotPos);
			variable = s.substr(dotPos + 1);
		} else {
			variable = s;
		}
	}

	const std::string &getType() const { return type; }

	const std::string &getVariable() const { return variable; }

	static std::vector<TypeVariable> makeList(std::string s) {
		std::vector<TypeVariable> list;
		std::string::const_iterator iter = std::begin(s);
		std::string::const_iterator end = std::end(s);
		std::string type_var;
		while (iter != end) {
			std::string::size_type commaPos = std::string(iter, end).find(',');
			if (commaPos == std::string::npos) {
				type_var = std::string(iter, end);
				iter = end;
			} else {
				type_var = std::string(iter, iter + commaPos);
			}
			list.emplace_back(TypeVariable(type_var));
			if (iter != end)
				iter = iter + commaPos + 2;
		}
		return list;
	}
};

//std::shared_ptr<object_list> scim_data::generateRelations(const std::string &scim_type) {
//
//	const config_file &conf = config_file::instance();
//
//	auto new_objects_map = std::make_shared<object_list>();
//
//	std::string type_variables = conf.get(scim_type + "-scim-variables");
//	TypeVariable uniqueIdentifier(conf.get(scim_type + "-unique-identifier"));
//
//	std::vector<TypeVariable> typeVars = TypeVariable::makeList(type_variables);
//	// iterate over all objects loaded from the database
//	for (auto &&object : list->objects) {
//
//		// grab the attribute holding the relation
//		const string_vector &relations = object.second->get_values(uniqueIdentifier.getVariable());
//		for (auto &&relation : relations) {
//			if (relation.empty())
//				continue;
//			base_object newRelation(scim_type);
//			newRelation.add_values(uniqueIdentifier.getVariable(), {relation});
//			for (auto &&typeVar : typeVars) {
//				if (typeVar.getVariable() != uniqueIdentifier.getVariable()) {
//					const string_vector &attributes = object.second->get_values(typeVar.getVariable());
//					newRelation.add_values(typeVar.getVariable(), attributes);
//				}
//			}
//			new_objects_map->add_object(relation, std::move(newRelation));
//		}
//		/** for all objects title, add objects id to the  */
//		for (const auto &o : object.second->get_values("title")) {
//			auto aGroup = new_objects_map->objects.find(o);
//			aGroup->second->add_values("members", object.second->get_values("uid"));
//			std::cout << "oj" << std::endl;
//		}
//	}
//
//	return new_objects_map;
//}


//void scim_data::findRelations() {
//
//	const config_file &conf = config_file::instance();
//
//	// find all ss12000 types requested and iterate over db objects
//	// to create related objects
//	std::vector<std::string> types = findTypes();
//
//
//	for (auto &&type : types) {
//		if (type == "User")
//			continue; // no need to find relations to myself
//		std::string type_variables = conf.get(type + "-scim-variables");
//		TypeVariable uniqueIdentifier(conf.get(type + "-unique-identifier"));
//
//		std::vector<TypeVariable> typeVars = TypeVariable::makeList(type_variables);
//		object_map_t new_objects_map;
//		// iterate over all objects loaded from the database
//		for (auto &&object : list->objects) {
//
//			// grab the attribute holding the relation
//			const string_vector &relations = object.second->get_values(uniqueIdentifier.getVariable());
//			if (!relations.empty()) {
//				for (auto &&relation : relations) {
//					if (relation.empty())
//						continue;
//					auto newRelation = std::make_shared<base_object>(type);
//					newRelation->add_attribute(uniqueIdentifier.getVariable(), {relation});
//					for (auto &&typeVar : typeVars) {
//						if (typeVar.getVariable() != uniqueIdentifier.getVariable()) {
//							const string_vector &attributes = object.second->get_values(typeVar.getVariable());
//							newRelation->add_attribute(typeVar.getVariable(), attributes);
//						}
//					}
//					new_objects_map.emplace(std::make_pair(relation, std::move(newRelation)));
//				}
//			}
//		}
//		for (auto &&item : new_objects_map) {
//			list->objects.emplace(std::make_pair(item.first, item.second));
//		}
//	}
//}


std::vector<std::string> scim_data::findTypes() {
	config_file &conf = config_file::instance();

	std::vector<std::string> types;

	std::for_each(conf.begin(), conf.end(), [&types](auto thing) {
		std::string::size_type type_end = thing.first.find("-scim-conf");
		if (type_end != std::string::npos) {
			std::string type(thing.first.substr(0, type_end));
			types.emplace_back(type);
		}
	});

	return types;
}

