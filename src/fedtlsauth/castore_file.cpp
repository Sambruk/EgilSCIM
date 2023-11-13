/*
 * Copyright (c) 2019 Föreningen Sambruk
 *
 * You should have received a copy of the MIT license along with this project.
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "castore_file.hpp"
#include <stdexcept>

#ifdef _WIN32
#include <stdio.h>
#else
#include <unistd.h>
#endif

namespace federated_tls_auth {

castore_file::castore_file() {
#ifdef _WIN32
    char name[L_tmpnam_s];
    errno_t err;

    err = tmpnam_s(name, L_tmpnam_s);

    if (err) {
        throw std::runtime_error("Failed to create temporary file name");
    }

    err = fopen_s(&f, name, "wb");

    if (err) {
        throw std::runtime_error("Failed to create temporary file");
    }

    path = name;
#else
    char template_path[] = "/tmp/fedtlsauthcastoreXXXXXX";
    fd = mkstemp(template_path);
    if (fd < 0) {
        throw std::runtime_error("Failed to create temporary file");
    }

    path = template_path;
#endif
}

castore_file::~castore_file() {
#ifdef _WIN32
    if (f) {
        fclose(f);
    }
    remove(path.c_str());
#else
    if (fd >= 0) {
        // Make sure it gets deleted when closed
        unlink(path.c_str());
        close(fd);
    }
#endif
}

void castore_file::write(const void* data, size_t count) {
#ifdef _WIN32
    if (!f) {
        errno_t err;

        err = fopen_s(&f, path.c_str(), "ab");

        if (err) {
            throw std::runtime_error("Failed to open temporary file for appending");
        }
    }
    auto res = fwrite(data, 1, count, f);

    if (res != count) {
        throw std::runtime_error("Failed to write to temporary file");
    }
    fclose(f);
    f = nullptr;
#else
    ssize_t res = ::write(fd, data, count);
    if (res != static_cast<ssize_t>(count)) {
        throw std::runtime_error("Failed to write to temporary file");
    }
#endif
}

std::string castore_file::get_path() const {
    return path;
}

} // namespace federated_tls_auth
