//
// Created by Ola Mattsson on 2018-08-16.
//

#ifndef SIMPLESCIM_UTILS_HPP
#define SIMPLESCIM_UTILS_HPP

#include <cstdlib>
#include <string>
#include <vector>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/lexical_cast.hpp>

std::string unifyurl(const std::string &s);

std::vector<std::string> string_to_vector(const std::string &s);

std::pair<std::string, std::string> string_to_pair(const std::string &s);

std::string pair_to_string(const std::pair<std::string, std::string> &pair);

std::string &toUpper(std::string &s);

std::string toUpper(const std::string &s);

void print_error();

void print_status(const char *config_file_name);

int check_params(int argc, char **argv);
std::string get_test_server_url(char **argv);

class uuid_util
{
	boost::uuids::random_generator generator;

	uuid_util() = default;
public:
	static uuid_util &instance() {
		static uuid_util gen;
		return gen;
	}
	std::string generate();
	std::string generate(const std::string &a, const std::string &b);


	std::string un_parse_uuid(char * val);

};

#endif //SIMPLESCIM_UTILS_HPP
