/**
 *  This file is part of the EGIL SCIM client.
 *
 *  Copyright (C) 2017-2026 Föreningen Sambruk
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

#ifndef EGILSCIMCLIENT_EXTERNAL_PROCESS_HPP
#define EGILSCIMCLIENT_EXTERNAL_PROCESS_HPP

#include <string>
#include <vector>
#include <filesystem>
#include <memory>
#include <iostream>

/**
 * Interface for receiving output data from an external process.
 */
class process_sink {
public:
    virtual ~process_sink() = default;
    virtual void write(const char* data, size_t len) = 0;
};

/**
 * Sink that forwards all data to std::cerr.
 */
class stderr_sink : public process_sink {
public:
    void write(const char* data, size_t len) override {
        std::cerr.write(data, len);
    }
};

/**
 * Sink that accumulates all data in a string.
 */
class string_sink : public process_sink {
public:
    void write(const char* data, size_t len) override {
        content.append(data, len);
    }
    std::string content;
};

/**
 * Sink that discards all data.
 */
class null_sink : public process_sink {
public:
    void write(const char*, size_t) override {}
};

/**
 * Configuration for a single external process session.
 */
struct external_process_session {
    std::string name;
    std::string command;
    std::string init;
    std::string cleanup;
    std::string secret;
    std::filesystem::path temp_dir;
};

/**
 * Manages external process sessions: temp directories, init/cleanup lifecycle,
 * and command execution.
 */
class external_process_manager {
public:
    /**
     * Constructs the manager with an error sink for reporting errors
     * from init/cleanup commands and the destructor. The caller must
     * ensure the sink outlives the manager.
     */
    explicit external_process_manager(process_sink& error_sink)
        : error_sink_(error_sink) {}
    ~external_process_manager();

    external_process_manager(const external_process_manager&) = delete;
    external_process_manager& operator=(const external_process_manager&) = delete;

    /**
     * Parses session definitions from a JSON string (the value of
     * external-process-sessions config variable) and creates temp
     * directories.
     */
    void parse_sessions(const std::string& json);

    /**
     * Runs init commands for all sessions that have one.
     * Should be called before any data loading.
     */
    void init_sessions();

    /**
     * Runs cleanup commands for all sessions that have one,
     * then removes all temp directories.
     */
    void cleanup_sessions();

    /**
     * Runs the command for the named session with extra arguments.
     * Output from the process is forwarded to the provided sinks.
     * Returns the exit code of the child process.
     * Throws std::runtime_error if the session name is not found.
     */
    int run_command(const std::string& session_name,
                    const std::string& extra_args,
                    process_sink& stdout_sink,
                    process_sink& stderr_sink) const;

    /**
     * Gets a session by name. Throws std::runtime_error if not found.
     */
    const external_process_session& get_session(const std::string& name) const;

    /**
     * Builds the full command line for a session command, appending
     * extra_args and --secret if configured.
     */
    static std::string build_command_line(const std::string& base_command,
                                          const std::string& extra_args,
                                          const std::string& secret);

private:
    std::vector<external_process_session> sessions_;
    bool cleaned_up_ = false;
    process_sink& error_sink_;

    /**
     * Runs an external command as a child process.
     *
     * The command_line string is split into a program and arguments
     * (on whitespace) and executed directly — no shell is involved.
     * If shell features are needed, specify the shell as the program
     * (e.g. "/bin/sh -c '...'" or "cmd /c ...").
     *
     * The child process runs with working_dir as its working directory.
     *
     * Output from the child's stdout and stderr is forwarded to the
     * provided sinks as data arrives. Both are read concurrently
     * to avoid deadlock.
     *
     * Stdin is kept open for the duration of the process and closed
     * after stdout and stderr have been fully read.
     *
     * Blocks until the child process exits. Returns the exit code.
     *
     * Throws std::runtime_error if the process could not be started.
     */
    int run_process(const std::string& command_line,
                    const std::filesystem::path& working_dir,
                    process_sink& stdout_sink,
                    process_sink& stderr_sink) const;
};

#endif // EGILSCIMCLIENT_EXTERNAL_PROCESS_HPP
