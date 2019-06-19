#ifndef FEDTLSAUTH_CASTORE_FILE_HPP
#define FEDTLSAUTH_CASTORE_FILE_HPP

#include <stddef.h>
#include <string>

namespace FederatedTLSAuth {

class CAStoreFile {
public:
    // Creates a temporary file we can write to
    CAStoreFile();

    // Closes and deletes the file
    ~CAStoreFile();

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

} // namespace FederatedTLSAuth

#endif // FEDTLSAUTH_CASTORE_FILE_HPP
