//
// Created by Ola Mattsson on 2018-09-01.
//

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
