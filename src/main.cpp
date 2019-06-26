/**
 * Copyright © 2017-2018  Max Wällstedt <>
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

#include <iostream>
#include <boost/program_options.hpp>
#include <experimental/filesystem>
#include <vector>
#include <string>

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

namespace po = boost::program_options;
namespace filesystem = std::experimental::filesystem;

void print_usage(const std::string& program_name,
                 const po::options_description& options) {
    std::cout << "Usage: " << program_name << " [OPTIONS] <config-file1> [config-file2...]\n\n";
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

int main(int argc, char *argv[]) {
    try {
        po::options_description cmdline_options("All options");
        po::options_description generic("Options");
        po::options_description hidden("Hidden options");

        generic.add_options()
            ("help,h", "produce help message")
            ("version,v", "displays version of this program");

        // Config file variables exposed as command line options
        std::vector<config_file_option> common_vars =
            { { "cert",             "client certificate",               true },
              { "key",              "client private key",               true },
              { "cache-file",       "cache file",                       true },
              { "metadata-path",    "metadata file",                    true },
              { "metadata-entity",  "entity in metadata to connect to", false },
              { "metadata-server",  "name of server to connect to",     false }
            };

        for (const auto& var : common_vars) {
            generic.add_options()
                (var.name.c_str(), po::value<std::string>(), var.description.c_str());
        }

        hidden.add_options()
            ("config-file", po::value<std::vector<std::string>>(), "config file");

        cmdline_options.add(generic).add(hidden);

        po::positional_options_description p;
        p.add("config-file", -1);

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

        std::vector<std::string> files;

        if (vm.count("config-file")) {
            files = vm["config-file"].as<std::vector<std::string>>();
        }
        else {
            print_usage(argv[0], generic);
            return EXIT_FAILURE;
        }

        for (const auto& file : files) {
            /** Load configuration file */
            std::cout << "processing: " << file << std::endl;
            int err = 0;
            try {
                err = config.load(file);
            } catch (std::string& msg) {
                std::cerr << msg << std::endl;
                return EXIT_FAILURE;
            }

            for (auto& var : common_vars) {
                if (vm.count(var.name)) {
                    auto value{vm[var.name].as<std::string>()};
                    if (var.path) {
                        value = filesystem::absolute(value);
                    }
                    config.replace_variable(var.name, value);
                }
            }

            if (err == -1) {
                print_error();
                continue;
            }

            /** Get objects from LDAP catalogue */
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
                continue;
            }


            /** Get objects from cache file */
            std::shared_ptr<object_list> cache = cache_file::instance().get_contents();

            if (cache == nullptr) {
                print_error();
                server.clear();
                config.clear();
                continue;
            }

            /** Perform SCIM operations */
            try {
                err = ScimActions(SCIMServerInfo(config)).perform(server, *cache);
            } catch (const std::string& err_msg) {
                std::cerr << err_msg << std::endl;
            }

            if (err == -1) {
                server.clear();
                print_error();
                config.clear();
                continue;
            }

            print_status(file.c_str());
            print_error();

            /* Clean up */
            config.clear();
            server.clear();
        }
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
