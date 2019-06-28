/*
 * Copyright (c) 2019 Föreningen Sambruk
 *
 * You should have received a copy of the MIT license along with this project.
 * If not, see <https://opensource.org/licenses/MIT>.
 */

#ifndef FEDTLSAUTH_CASTORE_FILE_HPP
#define FEDTLSAUTH_CASTORE_FILE_HPP

#include <stddef.h>
#include <string>

namespace federated_tls_auth {

/*
 * A temporary file used as CA store during authentication.
 *
 * The temporary file is automatically deleted when the
 * castore_file object is deleted.
 */
class castore_file {
public:
    // Creates a temporary file we can write to
    castore_file();

    // Closes and deletes the file
    ~castore_file();

    /*
     * Write some data to the file.
     * Throws runtime_error on failure.
     */
    void write(const void* data, size_t count);

    // Full path to the file
    std::string get_path() const;
private:
    // The file descriptor
    int fd;

    // The full path of the file
    std::string path;
};

} // namespace federated_tls_auth

#endif // FEDTLSAUTH_CASTORE_FILE_HPP
