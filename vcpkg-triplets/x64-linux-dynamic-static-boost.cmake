# This triplet uses dynamic linking by default on Linux, but static linking for Boost.
#
# One reason to use dynamic linking is to avoid problems that mixing OpenSSL versions
# can lead to (if our statically linked OpenSSL has a different version than what is
# dynamically loaded, for instance by an ODBC driver).
# A reason to use static linking for Boost is that the version included in Debian keeps
# changing.
set(VCPKG_TARGET_ARCHITECTURE x64)
set(VCPKG_CMAKE_SYSTEM_NAME Linux)

set(VCPKG_CRT_LINKAGE dynamic)
set(VCPKG_LIBRARY_LINKAGE dynamic)

if(PORT MATCHES "^boost-")
    set(VCPKG_LIBRARY_LINKAGE static)
endif()
