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
 * Result of running an external process command.
 */
struct process_result {
    std::string stdout_content;
    std::string stderr_content;
    int exit_code = -1;
};

/**
 * Manages external process sessions: temp directories, init/cleanup lifecycle,
 * and command execution.
 */
class external_process_manager {
public:
    external_process_manager() = default;
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
     * Runs the command for the named session with extra arguments and
     * returns the captured stdout (JSON data), stderr, and exit code.
     * Throws std::runtime_error if the session name is not found.
     */
    process_result run_command(const std::string& session_name,
                               const std::string& extra_args) const;

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
     * If capture_stdout is true, the child's stdout is captured and
     * returned in process_result::stdout_content. If false, stdout
     * is inherited (shown in the terminal).
     *
     * The child's stderr is always captured and returned in
     * process_result::stderr_content.
     *
     * Stdin is kept open for the duration of the process and closed
     * after stdout and stderr have been fully read.
     *
     * Blocks until the child process exits. The exit code is returned
     * in process_result::exit_code.
     *
     * Throws std::runtime_error if the process could not be started.
     */
    process_result run_process(const std::string& command_line,
                               const std::filesystem::path& working_dir,
                               bool capture_stdout) const;
};

#endif // EGILSCIMCLIENT_EXTERNAL_PROCESS_HPP
