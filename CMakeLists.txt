cmake_minimum_required(VERSION 3.12)
project(EgilSCIMClient CXX)

set(EgilSCIM_VERSION_MAJOR 2)
set(EgilSCIM_VERSION_MINOR 23)
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

find_package(Boost CONFIG REQUIRED COMPONENTS program_options uuid interprocess)
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
file(GLOB_RECURSE SOURCES
        "${PROJECT_SOURCE_DIR}/${srcroot}/*.cpp"
        "${PROJECT_SOURCE_DIR}/${srcroot}/*.hpp"
)
list(FILTER SOURCES EXCLUDE REGEX ".*/${srcroot}/tests/.*")
list(FILTER SOURCES EXCLUDE REGEX ".*/main\.cpp$")

file(GLOB TEST_SOURCES "${PROJECT_SOURCE_DIR}/${srcroot}/tests/*.cpp")

add_library (Egil ${SOURCES})

add_executable(EgilSCIMClient ${srcroot}/main.cpp)
add_executable(tests ${TEST_SOURCES})

target_link_libraries(EgilSCIMClient LINK_PUBLIC Egil Boost::program_options Boost::uuid CURL::libcurl Boost::interprocess)
target_link_libraries(tests LINK_PUBLIC Egil)

install (TARGETS EgilSCIMClient DESTINATION bin)

