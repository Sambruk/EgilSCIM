/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2025 Föreningen Sambruk
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
#define _SILENCE_EXPERIMENTAL_FILESYSTEM_DEPRECATION_WARNING
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
#include "rendered_cache_file.hpp"
#include "scim.hpp"
#include "json_data_file.hpp"
#include "utility/utils.hpp"
#include "data_server.hpp"
#include "scim_server_info.hpp"
#include "post_processing.hpp"
#include "sql.hpp"
#include "generated_group_load.hpp"
#include "thresholds.hpp"
#include "print_cache.hpp"
#include "generated_organisation_load.hpp"

namespace po = boost::program_options;
namespace filesystem = std::experimental::filesystem;

// TODO: Add the rest of the options names here
namespace options {
    const char* CACHE_FILE = "cache-file";

    const char* USER_BLACKLIST_FILE = "user-blacklist-file";
    const char* USER_BLACKLIST_ATTRIBUTE = "user-blacklist-attribute";

    const char* PRINT_CACHE = "print-cache";
    const char* PRINT_CACHE_BY_ENDPOINT = "print-cache-by-endpoint";
    const char* PRINT_CACHE_TYPE = "print-cache-type";
    const char* PRINT_CACHE_WHERE = "print-cache-where";
}

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

// Class responsible for writing the status file.
// The file is written in the destructor to make sure
// that we write the status file regardless of how we
// exit main().
class status_writer {
public:
    status_writer(const std::string& path, time_t start)
    : file(path), start_time(start) {}

    ~status_writer();

    void set_synced_objects(std::shared_ptr<rendered_object_list> objects) {
        synced_objects = objects;
    }

private:
    std::string file;
    time_t start_time;
    std::shared_ptr<rendered_object_list> synced_objects; // can be nullptr
};

// Writes the status JSON file
status_writer::~status_writer() {
    time_t end_time = time(nullptr);
    int duration = int(end_time-start_time);
    std::map<std::string, int> resourceCounts;

    if (synced_objects) {
        for (const auto &iter : *synced_objects) {
            auto object = iter.second;

            resourceCounts[object->get_type()]++;
        }
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
 * Modifies a set of objects so they will be considered different from what's in the data source,
 * used by --force-update.
 */
void make_dirty(std::shared_ptr<rendered_object_list> objects, const std::vector<std::string>& uuids) {
    for (auto uuid : uuids) {
        auto obj = objects->get_object(uuid);

        if (obj) {
            auto new_obj = std::make_shared<rendered_object>(obj->get_id(), obj->get_type(), dummy_SCIM_object(obj->get_id(), "force update"));
            objects->remove_object(uuid);
            objects->add_object(new_obj);
        }
    }
}

/**
 * Reads the contents of the cache file, either in new or old format.
 * 
 * If the cache file was in the old format, the objects are rendered with
 * the current templates before being returned.
 * 
 * Returns an empty list if the cache file doesn't exist, but returns
 * nullptr if there is no cache file path setting in the config file.
 * 
 * When deciding whether or not to apply thresholds, we want to differentiate
 * between an empty cache and a non-existent cache, so the cache_file_existed
 * parameter will let the caller know if there was a cache file or not.
 */
std::shared_ptr<rendered_object_list> read_cache(const post_processing::plugins& ppp, bool *cache_file_existed) {
    auto cache_path = config_file::instance().get_path(options::CACHE_FILE);

    if (cache_path.empty()) {
        return nullptr;
    }

    *cache_file_existed = std::experimental::filesystem::exists(cache_path);

    try {
        return rendered_cache_file::get_contents(cache_path);
    } catch (const rendered_cache_file::bad_format&) {
        // Probably an old cache file, we'll try to convert it below
    } catch (const std::runtime_error& e) {
        std::cerr << "Failed to read cache file: " << e.what() << std::endl;
        return nullptr;
    }

    cache_file old_cache_file;
    std::shared_ptr<object_list> object_cache = old_cache_file.get_contents();

    if (object_cache == nullptr) { 
        return nullptr;
    }

    renderer rend;
    auto rendered_cache = std::make_shared<rendered_object_list>();
    for (const auto &itr : *object_cache) {
        try {
            rendered_cache->add_object(rend.render(ppp, *itr.second));
        } catch (const std::runtime_error&) {
            rendered_cache->add_object(std::make_shared<rendered_object>(itr.first, itr.second->getSS12000type(), ""));
        }
    }

    return rendered_cache;
}

int main(int argc, char *argv[]) {
    try {
        po::options_description cmdline_options("All options");
        po::options_description generic("Options");
        po::options_description hidden("Hidden options");

        generic.add_options()
            ("help,h",                         "produce help message")
            ("version,v",                      "displays version of this program")
            ("rebuild-cache,r",                "ignores cache file contents and instead queries SCIM server for list of objects")
            ("skip-load",                      "don't read from data source, causes delete for all objects in cache")
            ("skip-thresholds",                "don't verify thresholds")
            (options::PRINT_CACHE,             "prints contents of cache file (see --print-cache-type and --print-cache-where)")
            (options::PRINT_CACHE_BY_ENDPOINT, "uses the SCIM endpoints instead of EGIL types when printing the cache file");

        // Config file variables exposed as command line options
        std::vector<config_file_option> common_vars =
            { { "cert",             "client certificate",               true },
              { "key",              "client private key",               true },
              { options::CACHE_FILE,"cache file",                       true },
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

        generic.add_options()
            (options::USER_BLACKLIST_FILE, po::value<std::string>(), "a file of users which shall be blocked from loading")
            (options::USER_BLACKLIST_ATTRIBUTE, po::value<std::string>(), "attribute (in the data source) to match against user blacklist file");
        
        generic.add_options()
            (options::PRINT_CACHE_TYPE, po::value<std::vector<std::string>>(), "only print given type(s)")
            (options::PRINT_CACHE_WHERE, po::value<std::vector<std::string>>(), "only print objects where attributes match given values");

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

        setup_default_organisation_config_variables();

        if (vm.count(options::PRINT_CACHE)) {
            bool by_endpoint = vm.count(options::PRINT_CACHE_BY_ENDPOINT);
            auto cache_path = config_file::instance().get_path(options::CACHE_FILE);

            std::shared_ptr<rendered_object_list> cache;
            try {
                cache = rendered_cache_file::get_contents(cache_path);
            }
            catch (const rendered_cache_file::bad_format &) {
                std::cerr << "Unrecognized cache file format" << std::endl;
                return EXIT_FAILURE;
            }
            catch (const std::runtime_error &e) {
                std::cerr << "Failed to read cache file: " << e.what() << std::endl;
                return EXIT_FAILURE;
            }

            std::vector<std::string> types, where;
            if (vm.count(options::PRINT_CACHE_TYPE)) {
                types = vm[options::PRINT_CACHE_TYPE].as<std::vector<std::string>>();
            }
            if (vm.count(options::PRINT_CACHE_WHERE)) {
                where = vm[options::PRINT_CACHE_WHERE].as<std::vector<std::string>>();
            }
            print_cache(cache, by_endpoint, types, where);
            return EXIT_SUCCESS;
        }

        std::unique_ptr<status_writer> status;
        if (config.has("status-file")) {
            status = std::make_unique<status_writer>(config.get("status-file"), start_time);
        }

        add_scim_vars_for_virtual_groups();

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

        std::shared_ptr<sql::plugin> sqlp;
        if (config.has("sql-plugin-path") && config.has("sql-plugin-name")) {
            try {
                sqlp = std::make_shared<sql::plugin>(config.get_path("sql-plugin-path"),
                                                     config.get("sql-plugin-name"));
            }
            catch (std::runtime_error &e)
            {
                std::cerr << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }

        if (vm.count(options::USER_BLACKLIST_FILE)) {
            auto user_blacklist_file = filesystem::absolute(vm[options::USER_BLACKLIST_FILE].as<std::string>()).u8string();
            std::string user_blacklist_attribute;
            if (vm.count(options::USER_BLACKLIST_ATTRIBUTE)) {
                user_blacklist_attribute = vm[options::USER_BLACKLIST_ATTRIBUTE].as<std::string>();
            }
            set_user_blacklist(user_blacklist_file, user_blacklist_attribute);
        }
        

        /** Get objects from data source */
        data_server &server = data_server::instance();
        bool skip_load = vm.count("skip-load");
        try {
            if (!skip_load && !server.load(sqlp)) {
                print_error();
                std::cerr << "Failed to load from data source" << std::endl;
                return EXIT_FAILURE;
            }
        }
        catch (const std::exception &e) {
            std::cerr << "Failed to load from data source: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }

        SCIMServerInfo server_info{config};
        ScimActions scim_actions{server_info};

        /** Get objects from cache file */
        auto cache_file_existed = false;
        std::shared_ptr<rendered_object_list> cache = read_cache(ppp, &cache_file_existed);

        // Until we have managed to create a new cache, use the historical information for the
        // status file (in case e.g. we fail to create a new cache, or an unexpected exception
        // makes us exit early).
        if (status) {
            status->set_synced_objects(cache);
        }

        if (cache == nullptr) {
            print_error();
            server.clear();
            config.clear();
            return EXIT_FAILURE;
        }

        // Possibly apply thresholds
        bool skip_thresholds = vm.count("skip-thresholds");
        if (cache_file_existed && !skip_load && !skip_thresholds) {
            try { 
                verify_thresholds(cache, server);
            }
            catch (const threshold_error& e) {
                // One of the thresholds were violated
                std::cerr << e.what() << std::endl;
                return EXIT_FAILURE;
            }
            catch (const std::runtime_error& e) {
                // Probably unparsable threshold
                std::cerr << e.what() << std::endl;
                return EXIT_FAILURE;
            }
        }

        std::vector<ScimActions::scim_object_ref> all_scim_objects;
        if (vm.count("rebuild-cache")) {
            try {
                all_scim_objects = scim_actions.get_all_objects_from_scim_server();
            } catch (const std::runtime_error& e) {
                std::cerr << "Failed to get objects from SCIM server (" << e.what() << ")" << std::endl;
                return EXIT_FAILURE;
            }
        }

        if (vm.count("force-update")) {
            auto uuids = vm["force-update"].as<std::vector<std::string>>();
            make_dirty(cache, uuids);
        }

        if (vm.count("force-create")) {
            auto uuids = vm["force-create"].as<std::vector<std::string>>();
            for (auto uuid : uuids) {
                if (server.has_object(uuid)) {
                    cache->remove_object(uuid);
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

        if (status) {
            status->set_synced_objects(scim_actions.get_new_cache());
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
    catch (const std::string& s) {
        // We shouldn't be throwing strings anymore, if we get here it's a bug
        std::cerr << "Unexpected string caught: " << s << std::endl;
        return EXIT_FAILURE;
    }
    catch (...) {
        std::cerr << "Unexpected exception caught!" << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
