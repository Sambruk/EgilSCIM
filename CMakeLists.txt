cmake_minimum_required(VERSION 3.12)
project(EgilSCIMClient CXX)

set(EgilSCIM_VERSION_MAJOR 2)
set(EgilSCIM_VERSION_MINOR 18)
set(EgilSCIM_VERSION_PATCH 0)

configure_file(
        "${PROJECT_SOURCE_DIR}/src/utility/EgilSCIM_config.h.in"
        "${PROJECT_BINARY_DIR}/EgilSCIM_config.h"
)

include_directories("${PROJECT_BINARY_DIR}")
include_directories("${PROJECT_SOURCE_DIR}/src")

if (APPLE)
    #all ldap deprecated on apple platforms
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
endif ()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")

if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -Wall")
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules")

find_package(CURL REQUIRED)
if (NOT CURL_FOUND)
    message(FATAL_ERROR "please install curl")
else()
    include_directories(${CURL_INCLUDE_DIRS})
endif ()

find_package(Boost REQUIRED COMPONENTS program_options uuid)
if (NOT Boost_FOUND)
    message(FATAL_ERROR "please install boost")
else ()
    include_directories(${Boost_INCLUDE_DIRS})
endif ()

if (WIN32)
    set(LDFLAGS wldap32.lib Bcrypt.lib ${CURL_LIBRARIES})
else()
    set(LDFLAGS -lldap -llber -lstdc++fs -ldl ${CURL_LIBRARIES})
endif()

link_libraries(${LDFLAGS})

set(srcroot src)
set(SOURCES ${srcroot}/scim_json_parse.cpp ${srcroot}/utility/simplescim_error_string.cpp ${srcroot}/simplescim_scim_send.cpp ${srcroot}/cache_file.cpp ${srcroot}/model/base_object.cpp ${srcroot}/model/object_list.cpp ${srcroot}/model/object_list.hpp ${srcroot}/simplescim_ldap_attrs_parser.cpp ${srcroot}/config_file.cpp ${srcroot}/config_file_parser.cpp ${srcroot}/scim.cpp ${srcroot}/utility/utils.cpp ${srcroot}/utility/utils.hpp ${srcroot}/utility/utils.cpp ${srcroot}/ldap_wrapper.cpp ${srcroot}/ldap_wrapper.hpp ${srcroot}/data_server.cpp ${srcroot}/data_server.hpp ${srcroot}/json_data_file.cpp ${srcroot}/json_data_file.hpp ${srcroot}/json_template_parser.cpp ${srcroot}/json_template_parser.hpp ${srcroot}/simplescim_ldap.cpp ${srcroot}/generated_load.cpp ${srcroot}/generated_load.hpp ${srcroot}/generated_group_load.cpp ${srcroot}/generated_group_load.hpp ${srcroot}/csv_load.cpp ${srcroot}/csv_load.hpp ${srcroot}/csv_store.cpp ${srcroot}/csv_store.hpp ${srcroot}/csv_file.cpp ${srcroot}/csv_file.hpp ${srcroot}/sql_load.cpp ${srcroot}/sql_load.hpp ${srcroot}/scim_server_info.cpp ${srcroot}/scim_server_info.hpp ${srcroot}/load_limiter.hpp ${srcroot}/load_limiter.cpp ${srcroot}/fedtlsauth/castore_file.hpp ${srcroot}/fedtlsauth/castore_file.cpp ${srcroot}/fedtlsauth/metadata_parser.hpp ${srcroot}/fedtlsauth/metadata_parser.cpp ${srcroot}/utility/indented_logger.hpp ${srcroot}/utility/indented_logger.cpp ${srcroot}/post_processing.hpp ${srcroot}/post_processing.cpp ${srcroot}/sql.hpp ${srcroot}/sql.cpp ${srcroot}/dl.hpp ${srcroot}/dl.cpp ${srcroot}/plugin_config.hpp ${srcroot}/plugin_config.cpp ${srcroot}/load_common.hpp ${srcroot}/load_common.cpp ${srcroot}/load_limiter_impl.hpp ${srcroot}/model/rendered_object.hpp ${srcroot}/model/rendered_object.cpp ${srcroot}/model/rendered_object_list.hpp ${srcroot}/model/rendered_object_list.cpp ${srcroot}/renderer.hpp ${srcroot}/renderer.cpp ${srcroot}/rendered_cache_file.hpp ${srcroot}/rendered_cache_file.cpp ${srcroot}/utility/temporary_umask.hpp ${srcroot}/transformer.hpp ${srcroot}/transformer.cpp ${srcroot}/transformer_impl.hpp ${srcroot}/transformer_impl.cpp ${srcroot}/readable_id.hpp ${srcroot}/readable_id.cpp ${srcroot}/config.hpp ${srcroot}/config.cpp ${srcroot}/dnpactivities.cpp  ${srcroot}/dnpactivities.hpp ${srcroot}/utility/simpleurl.cpp ${srcroot}/utility/simpleurl.hpp ${srcroot}/thresholds.hpp ${srcroot}/thresholds.cpp ${srcroot}/audit.hpp ${srcroot}/audit.cpp)

set(TEST_SOURCES ${srcroot}/tests/main.cpp ${srcroot}/tests/json_template_parser_tests.cpp ${srcroot}/tests/scim_query_tests.cpp ${srcroot}/tests/csv_tests.cpp ${srcroot}/tests/uuid_tests.cpp ${srcroot}/tests/scim_json_parse_tests.cpp ${srcroot}/tests/load_limiter_tests.cpp ${srcroot}/tests/object_list_tests.cpp ${srcroot}/tests/generated_group_tests.cpp ${srcroot}/tests/transformer_tests.cpp ${srcroot}/tests/dnpactivities_tests.cpp ${srcroot}/tests/thresholds_tests.cpp ${srcroot}/tests/audit_tests.cpp)

add_library (Egil ${SOURCES})

add_executable(EgilSCIMClient ${srcroot}/main.cpp)
add_executable(tests ${TEST_SOURCES})

target_link_libraries(EgilSCIMClient LINK_PUBLIC Egil Boost::program_options Boost::uuid CURL::libcurl)
target_link_libraries(tests LINK_PUBLIC Egil)

install (TARGETS EgilSCIMClient DESTINATION bin)

