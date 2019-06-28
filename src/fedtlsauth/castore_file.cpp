/*
 * Copyright (c) 2019 Föreningen Sambruk
 *
 * You should have received a copy of the MIT license along with this project.
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#include "castore_file.hpp"
#include <stdexcept>
#include <unistd.h>

namespace federated_tls_auth {

castore_file::castore_file() {
    char template_path[] = "/tmp/fedtlsauthcastoreXXXXXX";
    fd = mkstemp(template_path);
    if (fd < 0) {
        throw std::runtime_error("Failed to create temporary file");
    }

    path = template_path;
}

castore_file::~castore_file() {
    if (fd >= 0) {
        // Make sure it gets deleted when closed
        unlink(path.c_str());
        close(fd);
    }
}

void castore_file::write(const void* data, size_t count) {
    ssize_t res = ::write(fd, data, count);
    if (res != static_cast<ssize_t>(count)) {
        throw std::runtime_error("Failed to write to temporary file");
    }
}

std::string castore_file::get_path() const {
    return path;
}

} // namespace federated_tls_auth
