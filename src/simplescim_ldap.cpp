/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2019 Föreningen Sambruk
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as
 *  published by the Free Software Foundation, either version 3 of the
 *  License, or (at your option) any later version.

 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.

 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "simplescim_ldap.hpp"

#include <set>
#include "utility/simplescim_error_string.hpp"
#include "model/base_object.hpp"
#include "model/object_list.hpp"
#include "utility/utils.hpp"
#include "data_server.hpp"
#include "ldap_wrapper.hpp"
#include "load_common.hpp"


std::string
store_relation(base_object &generated_object,
               const std::pair<std::string, std::string> &part_type,
               const std::pair<std::string, std::string> &master_id);

/**
 * Construct the object list from the LDAP response.
 * On success, a pointer to the constructed list is
 * returned. On error, nullptr is returned and
 * simplescim_error_string is set to an appropriate
 * error message.
 */
std::shared_ptr<object_list> ldap_to_object_list(ldap_wrapper& ldap,
                                                 const std::string& type,
                                                 std::shared_ptr<load_limiter> limiter,
                                                 indented_logger& load_logger) {
  std::shared_ptr<object_list> objects;
  std::string uid;

  objects = std::make_shared<object_list>();
  std::shared_ptr<base_object> obj = ldap.first_object();
  {
      indented_logger::indenter indenter(load_logger);
      while (obj != nullptr) {
          uid = obj->get_uid();

          if (!uid.empty() && limiter->include(obj.get())) {
              objects->add_object(uid, obj);

              load_logger.log("Found " + type + " " + uid);
          }

          obj = ldap.next_object();
      }
  }
  load_related(type, objects, load_logger);
  return objects;
}  

/**
 * Reads user data from LDAP into a user list object
 * according to the configuration file and returns a
 * pointer it. On error, nullptr is returned and
 * simplescim_error_string is set to an appropriate error
 * message.
 */
std::shared_ptr<object_list> ldap_get(ldap_wrapper &ldap,
                                      const std::string &type,
                                      indented_logger& load_logger) {

    load_logger.log(std::string("Loading entries for type ") + type + " from LDAP");
    indented_logger::indenter indenter(load_logger);
    
    std::shared_ptr<object_list> objects;
    if (ldap.search(type, load_logger)) {
        auto limiter = get_limiter(type);            
        objects = ldap_to_object_list(ldap, type, limiter, load_logger);
    }
    
    return objects;
}
