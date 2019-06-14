/**
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
 */

#include "tempfile.hpp"
#include <stdexcept>
#include <unistd.h>

Tempfile::Tempfile() {
    char template_path[] = "/tmp/EGILScimClientXXXXXX";
    fd = mkstemp(template_path);
    if (fd < 0) {
        throw std::runtime_error("Failed to create temporary file");
    }

    path = template_path;
}

Tempfile::~Tempfile() {
    if (fd >= 0) {
        // Make sure it gets deleted when closed
        unlink(path.c_str());
        close(fd);
    }
}

void Tempfile::write(const void* data, size_t count) {
    ssize_t res = ::write(fd, data, count);
    if (res != static_cast<ssize_t>(count)) {
        throw std::runtime_error("Failed to write to temporary file");
    }
}

std::string Tempfile::get_path() const {
    return path;
}
