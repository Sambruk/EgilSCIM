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
#ifndef SIMPLESCIM_SCIM_H
#define SIMPLESCIM_SCIM_H

#include <map>
#include <string>
#include "config_file.hpp"
#include "data_server.hpp"
#include "scim_server_info.hpp"
#include "renderer.hpp"
#include "model/rendered_object_list.hpp"
#include <memory>

class base_object;

class object_list;

enum SCIMOperation {
    SCIM_CREATE,
    SCIM_DELETE,
    SCIM_UPDATE
};

enum SCIMOperationFailureType {
    SCIM_OTHER_FAILURE,
    SCIM_CONFLICT_FAILURE,
    SCIM_NOT_FOUND_FAILURE
};

class ScimActions {
public:
        /** A reference to an object in the SCIM server.
     *  Contains a UUID and an endpoint.
     *
     *  This type is used when rebuilding the cache and fetch
     *  all objects from the server. Since we fetch by endpoint,
     *  and not all SS12000 types have different endpoints
     *  (Students and Teachers both end up under Users), we
     *  won't easily know what type of object it is. But need to
     *  remember the endpoint so we can delete it if needed.
     */
    struct scim_object_ref {
        std::string uuid;
        std::string endpoint;

        scim_object_ref(std::string u, std::string e)
                : uuid(u),
                  endpoint(e) {
        }
    };

private:

    mutable renderer rend;

    std::shared_ptr<rendered_object_list> scim_new_cache;
    const config_file &conf = config_file::instance();
    const SCIMServerInfo& scim_server_info;

    void simplescim_scim_clear() const;

    int simplescim_scim_init() const;

    struct statistics {
        size_t n_copy = 0, n_copy_fail = 0;
        size_t n_create = 0, n_create_fail = 0;
        size_t n_update = 0, n_update_fail = 0;
        size_t n_delete = 0, n_delete_fail = 0;
    };

    void process_changes(const object_list& current,
                         const rendered_object_list& pre_rendered,
                         const rendered_object_list& cache,
                         const post_processing::plugins& ppp,
                         statistics& stats,
                         bool rebuild_cache,
                         const std::set<std::string>& all_scim_uuids);

    void process_deletes(const object_list& current,
                         const rendered_object_list& cache,
                         const std::string& type,
                         statistics& stats);

    void process_deletes_per_endpoint(const std::vector<std::string>& to_delete,
                                      const std::string& endpoint,
                                      statistics& stats,
                                      const std::string& type);    

    static void print_statistics(const std::string& type,
                                 const statistics& stats);

public:
    ScimActions(const SCIMServerInfo& si)
           : scim_server_info(si) {
        scim_new_cache = std::make_unique<rendered_object_list>();

        int err = simplescim_scim_init();

        if (err == -1) {
            throw std::runtime_error("Failed to init SCIM");
        }

	const auto audit_log_file_cfg_variable = "audit-log-file";
        if (config_file::instance().get(audit_log_file_cfg_variable, true) != "") {
	  auto audit_log_file = format_log_path(config_file::instance().get_path(audit_log_file_cfg_variable, true));

	  audit_log.open(audit_log_file, std::ios_base::out | std::ios_base::app);
	  if (audit_log.fail()) {
	    throw std::runtime_error("Failed to open audit log");
	  }
        }
    }

    ~ScimActions() {
        /* Clean up */
        simplescim_scim_clear();
    }    
    
    /**
     * Makes SCIM requests by comparing the two user lists and
     * reading JSON templates from the configuration file.
     * Updates (or creates) the cache file.
     *
     * If rebuilding the cache (rebuild_cache = true), this
     * function will ignore what's in cached and instead use
     * all_scim_objects. For all objects in current, we will
     * then either create or update (depending on if the object
     * already exists in the SCIM server). All objects in
     * all_scim_objects which aren't in current will be deleted.
     *
     * On success, zero is returned. On error, -1 is returned
     * and simplescim_error_string is set to an appropriate
     * error message.
     */
    int perform(const data_server &current,
                const rendered_object_list &cached,
                const post_processing::plugins& ppp,
                bool rebuild_cache,
                const std::vector<scim_object_ref>& all_scim_objects);

    class copy_func {
        const rendered_object &cached;
    public:
        explicit copy_func(const rendered_object &o) : cached(o) {}

        int operator()(const ScimActions &);
    };

    class create_func {
        const rendered_object &create;
    public:
        explicit create_func(const rendered_object &c) : create(c) {}

        int operator()(const ScimActions &, bool& conflict);
    };

    class update_func {
        const rendered_object &object;
    public:
        update_func(const rendered_object &o) : object(o)
            {}

        int operator()(const ScimActions &, bool& non_existent);
    };

    class delete_func {
        const rendered_object &object;
    public:
        explicit delete_func(const rendered_object &o) : object(o) {}

        int operator()(const ScimActions &, bool& non_existent);
    };
    
    /** Gets a list of all resources in the SCIM server.
     *  Will query the endpoints for the SS12000 types included in scim-type-send-order.
     *
     *  This function will throw std::runtime_error if we fail to get the resources
     *  from the SCIM server.
     */
    std::vector<scim_object_ref> get_all_objects_from_scim_server();

    std::shared_ptr<rendered_object_list> get_new_cache() { return scim_new_cache; }

    // The audit_log stream will be unopened if audit logging isn't configured.
    std::ofstream audit_log;
};


#endif
