# Installing SimpleSCIM

SimpleSCIM currently depends on the following C libraries:

* `uthash` (included in the repository for now) for hash tables.
* `libldap` from OpenLDAP for fetching identity information using LDAP.
* `json-c` to parse the JSON objects from the configuration file.
* `libcurl` to send the SCIM request.

To compile SimpleSCIM, execute the following command from the
SimpleSCIM root directory:

```
make
```

To install SimpleSCIM to the system, execute the following command
from the SimpleSCIM root directory:

```
make install
```

Installing SimpleSCIM to the system will most likely require root
access.

# Uninstalling SimpleSCIM

To uninstall SimpleSCIM from the system, execute the following
command from the SimpleSCIM root directory:

```
make uninstall
```

Uninstalling SimpleSCIM from the system will most likely require root
access.

To remove any file under the SimpleSCIM root directory generated when
compiling any part of SimpleSCIM, execute the following command from
the SimpleSCIM root directory:

```
make clean
```

# Distribution Specific Installation Instructions

## CentOS 7

    sudo yum update
    sudo yum install git autoconf automake libtool gcc-c++ openssl-devel gcc json-c-devel openldap-devel
    git clone git://github.com/curl/curl.git
    cd curl
    ./buildconf
    ./configure
    make
    sudo make install
    cd ..
    git clone git://github.com/MaxWallstedt/SimpleSCIM.git
    cd SimpleSCIM/src/
    sed -i -e "s/\-pedantic\ //g" Makefile
    make

Run SimpleSCIM with:

    LD_LIBRARY_PATH=/usr/local/lib SimpleSCIM file...

## Fedora 26 Workstation / Fedora 26 Server

    sudo dnf update
    sudo dnf install git gcc json-c-devel openldap-devel libcurl-devel
    git clone git://github.com/MaxWallstedt/SimpleSCIM.git
    cd SimpleSCIM/src/
    make

## Ubuntu 16.04.3 LTS / Ubuntu Server 16.04.3 LTS

    sudo apt-get update && sudo apt-get upgrade
    sudo apt-get install git make pkg-config libjson0 libjson0-dev libldap2-dev libcurl4-openssl-dev
    git clone git://github.com/MaxWallstedt/SimpleSCIM.git
    cd SimpleSCIM/src/
    make

## Debian 9.1.0 (stable)

    su -
    apt-get update && apt-get dist-upgrade
    apt-get install git make gcc pkg-config libjson-c-dev libldap2-dev libcurl4-openssl-dev
    git clone git://github.com/MaxWallstedt/SimpleSCIM.git
    cd SimpleSCIM/src/
    make
