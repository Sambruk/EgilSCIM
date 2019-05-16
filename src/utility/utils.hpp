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

bool startsWith(const std::string& s, const std::string& prefix);

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

  /**
   * Generates a UUID based on two strings, in
   * a repeatable deterministic way.
   *
   * Technically we'll create a UUID version 5 (SHA-1)
   * from the two strings concatenated, with the standard
   * UUID for object identifiers as namespace.
   */
  std::string generate(const std::string &a, const std::string &b);


  std::string un_parse_uuid(char * val);

};

#endif //SIMPLESCIM_UTILS_HPP
