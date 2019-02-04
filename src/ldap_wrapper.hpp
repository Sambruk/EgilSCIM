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

#ifndef EGILSCIMCLIENT_LDAP_WRAPPER_HPP
#define EGILSCIMCLIENT_LDAP_WRAPPER_HPP

#include "model/object_list.hpp"

class ldap_wrapper {
  struct Impl;
  std::unique_ptr<Impl> impl;
  
public:

  explicit ldap_wrapper();
  ldap_wrapper(const ldap_wrapper&) = delete;
  ldap_wrapper& operator=(const ldap_wrapper&) = delete;

  ~ldap_wrapper();

  void ldap_close();

  bool valid();

  /**
   * Performs the LDAP search operation.
   */
  bool search(const std::string &type, const std::pair<std::string, std::string> &filters = {"", ""});

  /**
   * Begins iteration over LDAP results from a search.
   * Returns the first result as a base_object, or nullptr if
   * there was an error or zero results.
   */
  std::shared_ptr<base_object> first_object();

  /**
   * Continues an interation over LDAP results from a search.
   * Returns the next result as a base_object, or nullptr if
   * there was an error or zero results.
   */
  std::shared_ptr<base_object> next_object();
};


#endif //EGILSCIMCLIENT_LDAP_WRAPPER_HPP
