# Building EgilSCIM from source code

EgilSCIM currently depends on the following libraries:

* `libcurl` to send the SCIM request.
* `boost` (general purpose C++ libraries)
* `libldap` from OpenLDAP for fetching identity information using LDAP.

`libldap` is only used on Unix based platforms.

The build tool CMake is used to build the program.

The easiest way to get the required dependencies and make sure CMake knows
where they are, is to use the dependency manager vcpkg.

## Building with CMake and vcpkg on a Unix based system

Make sure the following is installed:

* C++ compiler and tool chain
* CMake
* vcpkg (you may need to set the VCPKG_ROOT environment variable so CMake finds it)
* libldap with header files (this dependency isn't managed by vcpkg)

Then you can execute the following commands to build the program:

```
cmake --preset=unix
cmake --build build
```

This should produce an EgilSCIMClient binary in the `build` directory.

## Building with CMake and vcpkg on a Windows based system

Make sure the following is installed:

* Microsoft Visual Studio (other compilers are likely to work as well)
* CMake
* vcpkg (you may need to set the VCPKG_ROOT environment variable so CMake finds it)

If you wish to use a different C++ toolchain you will need to modify the presets
in `CMakePresets.json`.

Then you can execute the following commands to build the program:

```
cmake --preset=windows
cmake --build build --config Release
```

This should produce an EgilSCIMClient.exe binary in the `build\Release` directory.
