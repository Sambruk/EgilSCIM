//
// Created by Ola Mattsson on 2018-09-17.
//

#ifndef EGILSCIMCLIENT_PERSISTER_HPP
#define EGILSCIMCLIENT_PERSISTER_HPP

#include <sqlite3.h>
#include <cstdio>
#include <string>
#include <optional>
#include <sstream>
#include "config_file.hpp"
#include "model/base_object.hpp"
#include <fstream>

/**
 * Example
 * Employment is EmpType1 and their pidSchoolUnit
 * 	store User.GUID and SchoolUnit.GUID 1..1 and generate a uuid for it
 *
 * 	persist base_object
 *
 *
 *  create table employment
 *  (
 *    employee_id     varchar(255),
 *    schoolunit_id   varchar(255),
 *    Employment_id   varchar(255) not null,
 *    employee_name   varchar(255),
 *    schoolUnit_name varchar(255)
 *  );
 *
 *  create unique index employment_employee_id_schoolunit_id_uindex
 *    on employment (employee_id, schoolunit_id);
 *
 */

class local_id_store {
	sqlite3 *db{};
	std::string db_file;
	bool db_is_ready = false;

	/**
	 * open the database, create it if necessary
	 * @param name
	 * @return
	 */
	bool open_db(const std::string &name);

	static int dummy_callback(void *NotUsed, int argc, char **argv, char **azColName);

	static int get_id_callback(void *id, int argc, char **argv, char **azColName);

public:
	local_id_store() {
		db_file = config_file::instance().get("local_id_db");
		open_db(db_file);
	}


	~local_id_store() {
		sqlite3_close(db);
	}

	bool is_open() {
		return db_is_ready;
	}

	std::optional<std::string> get_relational_id(const string_pair &index_fields);

	/**
	 * generates a UUID and stores it along with it's related id's
	 * @param index_fields
	 * @return the new UUID
	 */
	std::optional<std::string> create_relational_id(const string_pair &index_fields);
};

#endif //EGILSCIMCLIENT_PERSISTER_HPP
