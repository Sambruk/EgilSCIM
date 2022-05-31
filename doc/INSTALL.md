# Installing EgilSCIM

EgilSCIM currently depends on the following libraries:

* `libldap` from OpenLDAP for fetching identity information using LDAP.
* `libcurl` to send the SCIM request.
* `boost` (general purpose C++ libraries)

The build tool CMake is used to build the program.

See operating system specific documentation below for information
on how to install these dependencies.

To compile EgilSCIM, execute the following command from the
EgilSCIM root directory:

```
    cd build
    cmake ..
    make
```

This should result in the program file EgilSCIMClient being
created in the build directory.

You don't have to install the program to the system, it can be
run directly from the build directory.

To install EgilSCIM to the system, execute the following command
from the EgilSCIM root directory:

```
    sudo make install
```

Installing EgilSCIM to the system will most likely require root
access.

Further steps may be needed if you wish to use the utilities in
the ```tools``` directory. For instructions, see the README file
in that directory.

# Uninstalling EgilSCIM

To uninstall EgilSCIM from the system, execute the following
command from the EgilSCIM/build root directory:

```
sudo make uninstall
```

Uninstalling EgilSCIM from the system will most likely require root
access.

To remove any file under the EgilSCIM root directory generated when
compiling any part of EgilSCIM, execute the following command from
the EgilSCIM directory:

```
rm -rf build/*
```

# Running

Run EgilSCIM with:

```EgilSCIMClient supplier1.conf [supplier2.conf...]```

# System specific instructions

## Nix package manager
    cd <project-root>
    nix build

## Ubuntu Server 18.04 LTS
    sudo apt install cmake libldap2-dev libcurl4-openssl-dev libboost-all-dev
    cd EgilSCIM/build
    cmake ..
    make
    sudo make install

## Fedora 28
    sudo dnf install gcc-c++ cmake openldap-devel libcurl-devel boost-devel libuuid-devel
    cd EgilSCIM/build
    cmake ..
    make
    sudo make install
    
## MacOS X 10.13
    Install Xcode (tool chain is installed with Xcode)
    Install HomeBrew
    brew install cmake openssl openldap
    cd EgilSCIM/build
    cmake ..
    make

## Windows
The code can be built for Windows with Microsoft Visual C++ (and probably
other C++ compilers as well). You'll need to have header files and libraries
for Boost and CURL installed.

The program hasn't been tested much on Windows yet.
