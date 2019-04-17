# Installing EgilSCIM

EgilSCIM currently depends on the following libraries:

* `libldap` from OpenLDAP for fetching identity information using LDAP.
* `libcurl` to send the SCIM request.
* `boost` (general purpose C++ libraries)

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


Run EgilSCIM with:

    EgilSCIMClient file...

## Ubuntu 18.04
    sudo apt-get update && sudo apt-get upgrade
    sudo apt install git cmake pkg-config  libldap2-dev libcurl4-openssl-dev libboost-dev libsqlite3-dev uuid-dev
    git clone git://github.com/Sambruk/EgilSCIM.git
    cd EgilSCIM
    mkdir build
    cd build
    cmake ..
    make
    sudo make install

## Fedora 28
    sudo dnf install git gcc-c++ cmake openldap-devel libcurl-devel boost-devel sqlite-devel libuuid-devel
    git clone git://github.com/Sambruk/EgilSCIM.git
    cd EgilSCIM
    mkdir build
    cd build
    cmake ..
    make
    sudo make install
    
## MacOS X 10.13
    Install Xcode (tool chain is installed with Xcode)
    Install HomeBrew
    brew install cmake openssl openldap
    git clone git://github.com/Sambruk/EgilSCIM.git
    cd EgilSCIM
    mkdir build
    cd build
    cmake ..
    make

## openSUSE Leap 15.1

There is no official package included in Leap 15.1 for the Boost library,
other SUSE distributions may include Boost as an official package.
   
Fortunately it is easy to download from https://www.boost.org/ and there's
no installation required.

Assuming you've downloaded the EgilSCIM client to:

 * ```/home/user/EGIL/EgilSCIM```
 
and Boost to:

 * ```/home/user/EGIL/boost_1_70_0```

you can install required dependencies and build the client with these
commands:

```
sudo zypper install gcc-c++ cmake openldap2-devel libcurl-devel
cd /home/user/EGIL/EgilSCIM/build
BOOST_ROOT=/home/user/EGIL/boost_1_70_0/ cmake ..
make
```

(you can also set BOOST_ROOT as a permanent environment variable and
replace the cmake command above with simply ```cmake ..``)

## Windows
    Instructions not yet available. The source code does not currently compile on Windows.
