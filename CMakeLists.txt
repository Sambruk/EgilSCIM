cmake_minimum_required(VERSION 3.12)
project(EgilSCIMClient CXX)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wshadow -fexceptions -fsanitize=address")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-address-use-after-scope -Wshadow-uncaptured-local")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17 ")
#-DNDEBUG
find_package(Boost)
if (Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS})
endif ()

set(LDFLAGS -lldap -llber -ljson-c -lcurl -lssl -lcrypto -luuid -lsqlite3)

include_directories("${PROJECT_BINARY_DIR}" /usr/local/include/json-c /usr/local/Cellar/openldap/2.4.46/include /usr/local/opt/openssl/include)
link_libraries(${LDFLAGS})

set(srcroot src)
set(SOURCES ${srcroot}/scim_json_parse.cpp ${srcroot}/utility/simplescim_error_string.cpp ${srcroot}/simplescim_scim_send.cpp ${srcroot}/cache_file.cpp ${srcroot}/model/base_object.cpp ${srcroot}/simplescim_ldap_attrs_parser.cpp ${srcroot}/model/object_list.cpp ${srcroot}/config_file.cpp ${srcroot}/config_file_parser.cpp ${srcroot}/scim.cpp ${srcroot}/utility/utils.cpp ${srcroot}/utility/utils.hpp ${srcroot}/utility/utils.cpp ${srcroot}/ldap_wrapper.cpp ${srcroot}/ldap_wrapper.hpp ${srcroot}/data_server.cpp ${srcroot}/data_server.hpp ${srcroot}/local_id_store.cpp ${srcroot}/local_id_store.hpp ${srcroot}/scim_data.cpp ${srcroot}/scim_data.hpp ${srcroot}/json_data_file.cpp ${srcroot}/json_data_file.hpp)


add_executable(EgilSCIMClient ${SOURCES} ${srcroot}/simplescim_ldap.cpp ${srcroot}/main.cpp)

#add_executable(EgilSCIMClientSimulated ${SOURCES} ${srcroot}/simplescim_ldap_simulate.cpp ${srcroot}/main.cpp)
#add_executable(EgilSCIMClientPrint ${SOURCES} ${srcroot}/utility/simplescim_cache_file_print.cpp)
#add_executable(testserver test/test_server/test_server.cpp)