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

#include <vector>
#include <iostream>
#include <algorithm>

#include "utils.hpp"
#include "EgilSCIM_config.h"
#include "simplescim_error_string.hpp"

void print_usage(const std::string &app_name) {
  std::cout << app_name
	    << " Version "
	    << EgilSCIM_VERSION_MAJOR << '.'
	    << EgilSCIM_VERSION_MINOR << std::endl;
  std::cout << "Usage: "
	    << app_name
	    << " supplier.conf supplier.conf" << std::endl;
}

int check_params(int argc, char **argv) {
    if (argc < 2) {
        print_usage(argv[0]);
        return -1;
    }
    if (argc >= 2) {
        std::string first_param = argv[1];
        if (first_param == "-v" || first_param == "--version") {
            print_usage(argv[0]);
            return -1;
        } else if (first_param.find("--test_server_url") != std::string::npos) {
            return 1;
        }
    }
    return 0;
}

std::string get_test_server_url(char **argv) {
    std::string s = argv[1];
    return s.substr(s.find('=') + 1);
}

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
    std::transform(s.begin(), s.end(), out.begin(), ::toupper);
    return out;
}

std::string &toUpper(std::string &s) {
    std::transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

bool startsWith(const std::string& s, const std::string& prefix) {
	return s.substr(0, prefix.size()) == prefix;
}

std::string uuid_util::generate() {
    boost::uuids::uuid uuid = generator();
    std::string id = boost::uuids::to_string(uuid);
    return toUpper(id);
}

std::string uuid_util::generate(const std::string &a, const std::string &b) {
#if BOOST_VERSION < 106700
    // Older versions of Boost don't define name_generator_sha1 or
    // boost::uuids::ns::oid().
    // name_generator uses SHA1 but it's deprecated in newer Boost-versions.
    boost::uuids::uuid oid = {{
        0x6b, 0xa7, 0xb8, 0x12, 0x9d, 0xad, 0x11, 0xd1 ,
        0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8 }};
    boost::uuids::name_generator name_generator(oid);
#else
    boost::uuids::name_generator_sha1 name_generator(boost::uuids::ns::oid());
#endif

    boost::uuids::uuid uuid = name_generator(a + b);
    std::string id = boost::uuids::to_string(uuid);
    return toUpper(id);
}

std::string uuid_util::un_parse_uuid(char *val) {
    boost::uuids::uuid uuid{};
    ::memcpy(&uuid, val, 16);
    auto st = boost::lexical_cast<std::string>(uuid);
    return toUpper(st);
}
