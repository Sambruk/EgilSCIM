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

#include <iostream>
#include <boost/program_options.hpp>
#include <experimental/filesystem>
#include <vector>
#include <string>
#include <time.h>

#include "EgilSCIM_config.h"
#include "utility/simplescim_error_string.hpp"
#include "model/object_list.hpp"
#include "config_file.hpp"
#include "simplescim_ldap.hpp"
#include "cache_file.hpp"
#include "scim.hpp"
#include "json_data_file.hpp"
#include "utility/utils.hpp"
#include "data_server.hpp"
#include "scim_server_info.hpp"
#include "post_processing.hpp"

namespace po = boost::program_options;
namespace filesystem = std::experimental::filesystem;

void print_usage(const std::string& program_name,
                 const po::options_description& options) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <config-file>\n\n";
    std::cout << options << "\n";
}    

/*
 * A config file option which can also be specified
 * on the command line.
 */
struct config_file_option {
    std::string name;
    std::string description;
    bool path;

    config_file_option(const std::string& n,
                       const std::string& d,
                       bool p)
            : name(n),
              description(d),
              path(p) {}
};

// Writes the status JSON file
void write_status(const std::string& file,
                  time_t start_time,
                  int duration,
                  std::shared_ptr<object_list> synced_objects) {

    std::map<std::string, int> resourceCounts;

    for (const auto &iter : *synced_objects) {
        auto object = iter.second;

        resourceCounts[object->getSS12000type()]++;
    }

    // TODO: Write JSON with boost::ptree instead?
    std::ofstream of(file);

    of << "{" << std::endl;
    of << "  \"startTime\": " << start_time << "," << std::endl;
    of << "  \"duration\": " << duration << "," << std::endl;
    of << "  \"resourceCounts\": {" << std::endl;

    bool first = true;
    for (const auto &iter : resourceCounts) {
        if (!first) {
            of << "," << std::endl;
        }
        first = false;
        of << "    \"" << iter.first << "\":" << iter.second;
    }

    of << "  }" << std::endl;
    of << "}" << std::endl;
}

/**
 * Splits a string with format "variable=value" into its parts.
 * Used on the command line for overriding config file variables.
 */
void parse_override(const std::string& override_str,
                    std::string& variable,
                    std::string& value) {
    auto pos = override_str.find('=');

    if (pos == std::string::npos) {
        throw std::runtime_error("Malformed variable override (" + override_str + ")");
    }

    variable = override_str.substr(0, pos);
    value = override_str.substr(pos+1);
}

/**
 * Modifies a set of objects so they will be considered different from what's in the data source,
 * used by --force-update.
 */
void make_dirty(std::shared_ptr<object_list> objects, const std::vector<std::string>& uuids) {
    for (auto uuid : uuids) {
        auto obj = objects->get_object(uuid);

        if (obj) {
            obj->add_attribute("_force_update", { std::to_string(time(NULL)) });
        }
    }
}

int main(int argc, char *argv[]) {
    try {
        po::options_description cmdline_options("All options");
        po::options_description generic("Options");
        po::options_description hidden("Hidden options");

        generic.add_options()
            ("help,h",             "produce help message")
            ("version,v",          "displays version of this program")
            ("rebuild-cache,r",    "ignores cache file contents and instead queries SCIM server for list of objects");

        // Config file variables exposed as command line options
        std::vector<config_file_option> common_vars =
            { { "cert",             "client certificate",               true },
              { "key",              "client private key",               true },
              { "cache-file",       "cache file",                       true },
              { "metadata-path",    "metadata file",                    true },
              { "metadata-entity",  "entity in metadata to connect to", false },
              { "metadata-server",
                "(obsolete) name of server to connect to",              false },
              { "scim-auth-WEAK",
                "whether server authentication should be disabled",     false },
              { "status-file",
                "file in which to write a summary of the run",          true },
              { "scim-type-send-order",
                "which SS12000 types to include "\
                "and in which order to send them",                      false },
            };

        for (const auto& var : common_vars) {
            generic.add_options()
                (var.name.c_str(), po::value<std::string>(), var.description.c_str());
        }

        generic.add_options()
            ("D", po::value<std::vector<std::string>>(), "generic config variable override " \
             "(e.g. --D ldap-passwd=secret)");

        generic.add_options()
            ("force-update", po::value<std::vector<std::string>>(), "update object even if it hasn't changed")
            ("force-create", po::value<std::vector<std::string>>(), "create object even if it's already in the cache");

        hidden.add_options()
            ("config-file", po::value<std::vector<std::string>>(), "config file");

        cmdline_options.add(generic).add(hidden);

        po::positional_options_description p;
        p.add("config-file", 1);

        po::variables_map vm;
        po::store(po::command_line_parser(argc, argv).
                  options(cmdline_options).positional(p).run(), vm);

        if (vm.count("help")) {
            print_usage(argv[0], generic);
            return EXIT_SUCCESS;
        }
        
        if (vm.count("version")) {
            std::cout << "EGIL SCIM client version "
                      << EgilSCIM_VERSION_MAJOR << "."
                      << EgilSCIM_VERSION_MINOR << "."
                      << EgilSCIM_VERSION_PATCH << "\n";
            return EXIT_SUCCESS;
        }
        
        try {
            po::notify(vm);
        }
        catch (const po::error& e) {
            std::cerr << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        
        config_file &config = config_file::instance();

        std::string config_file;

        if (vm.count("config-file")) {
            config_file = vm["config-file"].as<std::vector<std::string>>().at(0);
        }
        else {
            print_usage(argv[0], generic);
            return EXIT_FAILURE;
        }

        /** Load configuration file */
        std::cout << "processing: " << config_file << std::endl;

        time_t start_time = time(nullptr);
            
        int err = 0;
        try {
            err = config.load(config_file);
        } catch (std::string& msg) {
            std::cerr << msg << std::endl;
            return EXIT_FAILURE;
        }
            
        if (err == -1) {
            print_error();
            return EXIT_FAILURE;
        }

        for (auto& var : common_vars) {
            if (vm.count(var.name)) {
                auto value{vm[var.name].as<std::string>()};
                if (var.path) {
                    value = filesystem::absolute(value).u8string();
                }
                config.replace_variable(var.name, value);
            }
        }

        if (vm.count("D")) {
            auto overrides = vm["D"].as<std::vector<std::string>>();

            for (auto override_str : overrides) {
                std::string variable, value;
                parse_override(override_str, variable, value);
                config.replace_variable(variable, value);
            }
        }

        if (config.get_bool("scim-auth-WEAK")) {
            std::cout << "WARNING, running without authentication, this "
                "will either fail or the server might not be who you think it is. "
                "Use only for testing locally" << std::endl;
        }

        post_processing::plugins ppp;
        if (config.has("post-process-plugin-path")) {
            try {
                ppp = post_processing::load_plugins(config.get_path("post-process-plugin-path"),
                                                    config.get_vector("post-process-plugins", true));
            } catch (std::runtime_error& e) {
                std::cerr << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }

        /** Get objects from data source */
        data_server &server = data_server::instance();
        try {
            server.load();
        } catch (std::string& msg) {
            std::cerr << msg << std::endl;
            return EXIT_FAILURE;
        }
        if (server.empty()) {
            print_error();
            config.clear();
            server.clear();
            return EXIT_SUCCESS;
        }

        SCIMServerInfo server_info{config};
        ScimActions scim_actions{server_info};

        /** Get objects from cache file */
        std::shared_ptr<object_list> cache = cache_file::instance().get_contents();
        std::vector<ScimActions::scim_object_ref> all_scim_objects;
        if (vm.count("rebuild-cache")) {
            try {
                all_scim_objects = scim_actions.get_all_objects_from_scim_server();
            } catch (const std::runtime_error& e) {
                std::cerr << "Failed to get objects from SCIM server (" << e.what() << ")" << std::endl;
            }
        }
            
        if (cache == nullptr) {
            print_error();
            server.clear();
            config.clear();
            return EXIT_FAILURE;
        }

        if (vm.count("force-update")) {
            auto uuids = vm["force-update"].as<std::vector<std::string>>();
            make_dirty(cache, uuids);
        }

        if (vm.count("force-create")) {
            auto uuids = vm["force-create"].as<std::vector<std::string>>();
            for (auto uuid : uuids) {
                if (server.has_object(uuid)) {
                    cache->remove(uuid);
                }
                else {
                    std::cerr << "Can't force create " << uuid
                              << " which isn't in data source" << std::endl;
                }
            }
        }

        /** Perform SCIM operations */
        try {
            err = scim_actions.perform(server, *cache, ppp, vm.count("rebuild-cache"), all_scim_objects);
        } catch (const std::string& err_msg) {
            std::cerr << err_msg << std::endl;
        }

        if (err == -1) {
            server.clear();
            print_error();
            config.clear();
            return EXIT_FAILURE;
        }

        time_t end_time = time(nullptr);

        if (config.has("status-file")) {
            write_status(config.get("status-file"),
                         start_time,
                         int(end_time-start_time),
                         scim_actions.get_new_cache());
        }

        print_status(config_file.c_str());
        print_error();

        /* Clean up */
        config.clear();
        server.clear();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Unexpected exception caught!" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
