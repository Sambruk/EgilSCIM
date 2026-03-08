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

#include "external_process.hpp"

#include <iostream>
#include <sstream>
#include <future>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <cstring>
#endif

namespace pt = boost::property_tree;

external_process_manager::~external_process_manager() {
    if (!cleaned_up_) {
        try {
            cleanup_sessions();
        } catch (const std::exception& e) {
            auto msg = std::string("Exception during session cleanup: ") + e.what() + "\n";
            error_sink_.write(msg.c_str(), msg.size());
        } catch (...) {
            auto msg = std::string("Unknown exception during session cleanup\n");
            error_sink_.write(msg.c_str(), msg.size());
        }
    }
}

void external_process_manager::parse_sessions(const std::string& json) {
    try {
        std::stringstream json_stream;
        json_stream << json;
        pt::ptree root;
        pt::read_json(json_stream, root);

        for (auto& session_node : root) {
            external_process_session session;
            session.name = session_node.second.get<std::string>("name");

            for (const auto& existing : sessions_) {
                if (existing.name == session.name) {
                    throw std::runtime_error("Duplicate external process session name: \"" + session.name + "\"");
                }
            }

            session.command = session_node.second.get<std::string>("command");
            session.init = session_node.second.get<std::string>("init", "");
            session.cleanup = session_node.second.get<std::string>("cleanup", "");
            session.secret = session_node.second.get<std::string>("secret", "");

            // Create a unique temp directory for this session
            auto temp_base = std::filesystem::temp_directory_path();
            auto temp_dir = temp_base / ("egil_ep_" + std::to_string(std::hash<std::string>{}(session.name))
                                          + "_" + std::to_string(
#ifdef _WIN32
                                              GetCurrentProcessId()
#else
                                              getpid()
#endif
                                          ));
            std::filesystem::create_directories(temp_dir);
            session.temp_dir = temp_dir;

            sessions_.push_back(std::move(session));
        }
    } catch (const pt::ptree_error& e) {
        throw std::runtime_error(std::string("Failed to parse external-process-sessions: ") + e.what());
    }
}

void external_process_manager::init_sessions() {
    for (const auto& session : sessions_) {
        if (session.init.empty()) {
            continue;
        }

        auto cmd = build_command_line(session.init, "", session.secret);

        null_sink discard_stdout;
        auto exit_code = run_process(cmd, session.temp_dir, discard_stdout, error_sink_);

        if (exit_code != 0) {
            throw std::runtime_error("Init command for session \"" + session.name +
                                     "\" failed with exit code " + std::to_string(exit_code));
        }
    }
}

void external_process_manager::cleanup_sessions() {
    if (cleaned_up_) {
        return;
    }
    cleaned_up_ = true;

    for (const auto& session : sessions_) {
        if (!session.cleanup.empty()) {
            auto cmd = build_command_line(session.cleanup, "", session.secret);

            try {
                null_sink discard_stdout;
                auto exit_code = run_process(cmd, session.temp_dir, discard_stdout, error_sink_);
                if (exit_code != 0) {
                    auto msg = "Cleanup command for session \"" + session.name +
                               "\" failed with exit code " + std::to_string(exit_code) + "\n";
                    error_sink_.write(msg.c_str(), msg.size());
                }
            } catch (const std::exception& e) {
                auto msg = "Cleanup command for session \"" + session.name +
                           "\" threw exception: " + e.what() + "\n";
                error_sink_.write(msg.c_str(), msg.size());
            }
        }

        // Remove the temp directory
        std::error_code ec;
        std::filesystem::remove_all(session.temp_dir, ec);
        if (ec) {
            auto msg = "Failed to remove temp directory " + session.temp_dir.string() +
                       ": " + ec.message() + "\n";
            error_sink_.write(msg.c_str(), msg.size());
        }
    }
}

const external_process_session& external_process_manager::get_session(const std::string& name) const {
    for (const auto& session : sessions_) {
        if (session.name == name) {
            return session;
        }
    }
    throw std::runtime_error("External process session not found: " + name);
}

int external_process_manager::run_command(const std::string& session_name,
                                          const std::string& extra_args,
                                          process_sink& stdout_sink,
                                          process_sink& stderr_sink) const {
    const auto& session = get_session(session_name);
    auto cmd = build_command_line(session.command, extra_args, session.secret);
    return run_process(cmd, session.temp_dir, stdout_sink, stderr_sink);
}

std::string external_process_manager::build_command_line(const std::string& base_command,
                                                          const std::string& extra_args,
                                                          const std::string& secret) {
    std::string result = base_command;

    if (!extra_args.empty()) {
        result += " " + extra_args;
    }

    if (!secret.empty()) {
        result += " --secret=" + secret;
    }

    return result;
}

#ifdef _WIN32

namespace {

// RAII wrapper for Windows HANDLEs
struct handle_closer {
    void operator()(HANDLE h) const {
        if (h && h != INVALID_HANDLE_VALUE) CloseHandle(h);
    }
};
using unique_handle = std::unique_ptr<void, handle_closer>;

void read_pipe_to_sink(HANDLE pipe, process_sink& sink) {
    char buffer[4096];
    DWORD bytes_read;
    while (ReadFile(pipe, buffer, sizeof(buffer), &bytes_read, nullptr) && bytes_read > 0) {
        sink.write(buffer, bytes_read);
    }
}

} // anonymous namespace

int external_process_manager::run_process(const std::string& command_line,
                                          const std::filesystem::path& working_dir,
                                          process_sink& stdout_sink,
                                          process_sink& stderr_sink) const {

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = nullptr;

    // Create pipes for stdout and stderr
    HANDLE stdout_read = nullptr, stdout_write = nullptr;
    HANDLE stderr_read = nullptr, stderr_write = nullptr;
    HANDLE stdin_read = nullptr, stdin_write = nullptr;

    if (!CreatePipe(&stdout_read, &stdout_write, &sa, 0) ||
        !CreatePipe(&stderr_read, &stderr_write, &sa, 0) ||
        !CreatePipe(&stdin_read, &stdin_write, &sa, 0)) {
        throw std::runtime_error("Failed to create pipes for external process");
    }

    unique_handle h_stdout_read(stdout_read);
    unique_handle h_stdout_write(stdout_write);
    unique_handle h_stderr_read(stderr_read);
    unique_handle h_stderr_write(stderr_write);
    unique_handle h_stdin_read(stdin_read);
    unique_handle h_stdin_write(stdin_write);

    // Don't let the child inherit our end of the pipes
    SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stderr_read, HANDLE_FLAG_INHERIT, 0);
    SetHandleInformation(stdin_write, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA si = {};
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = stdout_write;
    si.hStdError = stderr_write;
    si.hStdInput = stdin_read;

    PROCESS_INFORMATION pi = {};

    // CreateProcessA needs a mutable command line string
    std::string cmd_copy = command_line;
    auto wd = working_dir.string();

    if (!CreateProcessA(nullptr, cmd_copy.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, wd.c_str(), &si, &pi)) {
        throw std::runtime_error("Failed to start external process: " + command_line +
                                 " (error " + std::to_string(GetLastError()) + ")");
    }

    unique_handle h_process(pi.hProcess);
    unique_handle h_thread(pi.hThread);

    // Close child-side pipe handles so we can detect when child exits
    h_stdout_write.reset();
    h_stderr_write.reset();
    h_stdin_read.reset();

    // Keep stdin open — closing it signals early termination to the child.
    // It will be closed by the unique_handle destructor after we're done.

    // Read stdout and stderr concurrently to avoid deadlock
    auto stderr_future = std::async(std::launch::async, [&]() {
        read_pipe_to_sink(stderr_read, stderr_sink);
    });

    read_pipe_to_sink(stdout_read, stdout_sink);

    stderr_future.get();

    // Close stdin now that we're done reading
    h_stdin_write.reset();

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exit_code;
    GetExitCodeProcess(pi.hProcess, &exit_code);

    return static_cast<int>(exit_code);
}

#else // Unix

namespace {

// Splits a command line string into tokens on whitespace.
std::vector<std::string> split_command_line(const std::string& cmd) {
    std::vector<std::string> tokens;
    std::string current;

    for (char c : cmd) {
        if (c == ' ') {
            if (!current.empty()) {
                tokens.push_back(std::move(current));
                current.clear();
            }
        } else {
            current += c;
        }
    }
    if (!current.empty()) {
        tokens.push_back(std::move(current));
    }
    return tokens;
}

} // anonymous namespace

int external_process_manager::run_process(const std::string& command_line,
                                          const std::filesystem::path& working_dir,
                                          process_sink& stdout_sink,
                                          process_sink& stderr_sink) const {

    int stdout_pipe[2], stderr_pipe[2], stdin_pipe[2];
    if (pipe(stdout_pipe) != 0 || pipe(stderr_pipe) != 0 || pipe(stdin_pipe) != 0) {
        throw std::runtime_error("Failed to create pipes for external process");
    }

    pid_t pid = fork();
    if (pid < 0) {
        throw std::runtime_error("Failed to fork for external process");
    }

    if (pid == 0) {
        // Child process
        close(stdout_pipe[0]);
        close(stderr_pipe[0]);
        close(stdin_pipe[1]);

        dup2(stdout_pipe[1], STDOUT_FILENO);
        dup2(stderr_pipe[1], STDERR_FILENO);
        dup2(stdin_pipe[0], STDIN_FILENO);

        close(stdout_pipe[1]);
        close(stderr_pipe[1]);
        close(stdin_pipe[0]);

        if (chdir(working_dir.c_str()) != 0) {
            _exit(127);
        }

        auto args = split_command_line(command_line);
        if (args.empty()) {
            _exit(127);
        }

        std::vector<char*> argv;
        for (auto& a : args) {
            argv.push_back(a.data());
        }
        argv.push_back(nullptr);

        execvp(argv[0], argv.data());
        _exit(127);
    }

    // Parent process
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);
    close(stdin_pipe[0]);

    // Keep stdin open — closing it signals early termination to the child.
    // We close it after reading is complete.

    // Read stdout and stderr concurrently to avoid deadlock
    auto read_fd_to_sink = [](int fd, process_sink& sink) {
        char buffer[4096];
        ssize_t n;
        while ((n = read(fd, buffer, sizeof(buffer))) > 0) {
            sink.write(buffer, n);
        }
        close(fd);
    };

    auto stderr_future = std::async(std::launch::async, read_fd_to_sink,
                                     stderr_pipe[0], std::ref(stderr_sink));

    read_fd_to_sink(stdout_pipe[0], stdout_sink);

    stderr_future.get();

    // Close stdin now that we're done reading — the child should be
    // finishing up. If it was waiting on stdin, this unblocks it.
    close(stdin_pipe[1]);

    int status;
    waitpid(pid, &status, 0);
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return -1;
}

#endif
