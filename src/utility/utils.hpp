//
// Created by Ola Mattsson on 2018-08-16.
//

#ifndef SIMPLESCIM_UTILS_HPP
#define SIMPLESCIM_UTILS_HPP

#include <cstdlib>
#include <string>
#include <vector>
std::string unifyurl(const std::string &s);

std::vector<std::string> string_to_vector(const std::string &s);

std::pair<std::string, std::string> string_to_pair(const std::string &s);

std::string pair_to_string(const std::pair<std::string, std::string> &pair);

std::string &toUpper(std::string &s);

std::string toUpper(const std::string &s);

void print_error();

void print_status(const char *config_file_name);


#endif //SIMPLESCIM_UTILS_HPP
