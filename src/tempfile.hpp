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

#ifndef EGILSCIM_TEMPFILE_HPP
#define EGILSCIM_TEMPFILE_HPP

#include <stddef.h>
#include <string>

class Tempfile {
public:
    // Creates a temporary file we can write to
    Tempfile();

    // Closes and deletes the file
    ~Tempfile();

    /*
     * Write some data to the file.
     * Throws runtime_error on failure.
     */
    void write(const void* data, size_t count);

    // Full path to the temporary file
    std::string get_path() const;
private:
    // The file descriptor
    int fd;

    // The full path of the file
    std::string path;
};

#endif // EGILSCIM_TEMPFILE_HPP
