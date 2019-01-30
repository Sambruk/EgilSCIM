# Installing EgilSCIM

EgilSCIM currently depends on the following C libraries:

* `libldap` from OpenLDAP for fetching identity information using LDAP.
* `libcurl` to send the SCIM request.

To compile EgilSCIM, execute the following command from the
EgilSCIM root directory:

```
    mkdir build
    cd build
    cmake ..
    make
```

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
rm -rf build
```


Run EgilSCIM with:

    LD_LIBRARY_PATH=/usr/local/lib EgilSCIMClient file...

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
    
## Windows
    Instructions not yet available, it's wip
