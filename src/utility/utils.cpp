//
// Created by Ola Mattsson on 2018-09-09.
//
#include <vector>
#include <iostream>
#include <algorithm>
#include "utils.hpp"
#include "simplescim_error_string.hpp"

std::vector<std::string> string_to_vector(const std::string &s) {
	if (s.empty())
		return {};
	std::vector<std::string> var_v{};
	auto cur = std::begin(s);
	auto end = std::end(s);
	auto var_end = std::find(cur, end, ' ');
	while (var_end != std::end(s)) {
		std::string variable(cur, var_end);
		std::string::size_type comma = variable.find(',');
		if (comma != std::string::npos)
			variable.erase(comma);
		var_v.emplace_back(variable);
		cur = var_end + 1;
		var_end = std::find(cur, end, ' ');
	}
	var_v.emplace_back(cur, end);
	return var_v;
}

std::pair<std::string, std::string> string_to_pair(const std::string &s) {
	auto cur = std::begin(s);
	auto end = std::end(s);
	auto var_end = std::find(cur, end, '.');
	if (var_end != end) {
		std::string second(var_end + 1, end);
		auto comma = second.find(',');
		if (comma != std::string::npos)
			second.erase(comma);
		return std::make_pair(std::string(cur, var_end), second);
	} else
		return std::make_pair("", "");

}

std::string pair_to_string(const std::pair<std::string, std::string> &pair) {
	return pair.first + '.' + pair.second;
}

void print_error() {
	if (has_errors_to_print())
		std::cerr << simplescim_error_string_get() << std::endl;
}

void print_status(const char *config_file_name) {
	std::cout << "Successfully performed SCIM operations for " << config_file_name << std::endl;
}

std::string unifyurl(const std::string &s) {
	std::string newString;
	auto iter = std::begin(s);
	auto end = std::end(s);
	while (iter != end) {
		switch (*iter) {
			case ' ':
				newString.append("%20");
				break;
			case '!':
				newString.append("%21");
				break;
			case '#':
				newString.append("%23");
				break;
			case '$':
				newString.append("%24");
				break;
			case '&':
				newString.append("%26");
				break;
			case '\'':
				newString.append("%27");
				break;
			case '(':
				newString.append("%28");
				break;
			case ')':
				newString.append("%29");
				break;
			case '*':
				newString.append("%2A");
				break;
			case '+':
				newString.append("%2B");
				break;
			case ',':
				newString.append("%2C");
				break;
			case '/':
				newString.append("%2F");
				break;
			case ':':
				newString.append("%3A");
				break;
			case ';':
				newString.append("%3B");
				break;
			case '=':
				newString.append("%3D");
				break;
			case '?':
				newString.append("%3F");
				break;
			case '@':
				newString.append("%40");
				break;
			case '[':
				newString.append("%5B");
				break;
			case ']':
				newString.append("%5D");
				break;
			default:
				newString += *iter;


		}
		iter++;
	}
	return newString;
}

std::string toUpper(const std::string &s) {
	std::string out(s);
	return toUpper(out);
}

std::string &toUpper(std::string &s) {
	for (auto &&c: s)
		c = static_cast<char>(std::toupper(c));
	return s;
}
