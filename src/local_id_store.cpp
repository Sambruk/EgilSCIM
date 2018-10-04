//
// Created by Ola Mattsson on 2018-09-17.
//

#include "local_id_store.hpp"
#include <string.h>



bool local_id_store::open_db(const std::string &name) {
	std::ifstream f(db_file);
	if (f.good()) {
		f.close();
		int rc = sqlite3_open(db_file.c_str(), &db);
		db_is_ready = rc == SQLITE_OK;
	} else {
		int rc = sqlite3_open(db_file.c_str(), &db);
		if (rc == SQLITE_OK) {
			std::stringstream buf;
			buf << "create table relations ("
			    << "first_id       varchar(255), "
			    << "second_id      varchar(255), "
			    << "relational_id  varchar(255) not null"
			    << "); "
			    << "create unique index relation_first_id_second_id_uindex "
			    << "on relations (first_id, second_id);";
			std::string statement = buf.str();
			char *zErrMsg = nullptr;
			rc = sqlite3_exec(db, statement.c_str(), dummy_callback, nullptr, &zErrMsg);
			if (rc == SQLITE_OK) {
				db_is_ready = true;
			} else {
				fprintf(stderr, "SQL error: %s\n", zErrMsg);
				sqlite3_free(zErrMsg);
			}
		} else {
			fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		}
	}
	return db_is_ready;
}

int local_id_store::dummy_callback(void *, int argc, char **argv, char **azColName) {
	int i;
	for (i = 0; i < argc; i++) {
		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	return 0;
}

int local_id_store::get_id_callback(void *id, int argc, char **argv, char **) {

	if (argc != 1) {
		std::cout << "query returned > 1 id. that's not right, this "
		          << "should be impossible unless the db has lost it's "
		          << "unique index. Check it will ya?" << std::endl;
	} else if (argv[0]) {
		strncpy((char *) id, argv[0], 36);
	}

	return 0;
}

std::optional<std::string> local_id_store::get_relational_id(const string_pair &index_fields) {
	std::stringstream buf;
	buf << "select relational_id from relations where"
	    << " first_id = \"" << index_fields.first << "\""
	    << "and second_id = \"" << index_fields.second << "\";";

	char newUUID[37];
	memset(newUUID, 0, sizeof(newUUID));
	char *zErrMsg = nullptr;
	std::string statement = buf.str();
	int rc = sqlite3_exec(db, statement.c_str(), get_id_callback, &newUUID, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		return {};
	}
	if (newUUID[0]) {
		return newUUID;
	} else {
		return {};
	}
}

std::optional<std::string> local_id_store::create_relational_id(const string_pair &index_fields) {
	std::string newUUID = uuid_util::instance().generate();

	std::stringstream buf;
	buf << "insert into relations "
	    << "(first_id, second_id, relational_id)"
	    << " values ("
	    << "\"" << index_fields.first << "\", "
	    << "\"" << index_fields.second << "\", "
	    << "\"" << newUUID << "\");";

	char *zErrMsg = nullptr;
	std::string statement = buf.str();
	int rc = sqlite3_exec(db, statement.c_str(), dummy_callback, nullptr, &zErrMsg);
	if (rc != SQLITE_OK) {
		fprintf(stderr, "SQL error: %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
		return {};
	}
	if (!newUUID.empty())
		return newUUID;
	else
		return {};
}
