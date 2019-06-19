#include "castore_file.hpp"
#include <stdexcept>
#include <unistd.h>

namespace FederatedTLSAuth {

CAStoreFile::CAStoreFile() {
    char template_path[] = "/tmp/fedtlsauthcastoreXXXXXX";
    fd = mkstemp(template_path);
    if (fd < 0) {
        throw std::runtime_error("Failed to create temporary file");
    }

    path = template_path;
}

CAStoreFile::~CAStoreFile() {
    if (fd >= 0) {
        // Make sure it gets deleted when closed
        unlink(path.c_str());
        close(fd);
    }
}

void CAStoreFile::write(const void* data, size_t count) {
    ssize_t res = ::write(fd, data, count);
    if (res != static_cast<ssize_t>(count)) {
        throw std::runtime_error("Failed to write to temporary file");
    }
}

std::string CAStoreFile::get_path() const {
    return path;
}

} // namespace FederatedTLSAuth
