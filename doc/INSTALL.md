# Installing SimpleSCIM

SimpleSCIM currently depends on the following C libraries:

* `libldap` from OpenLDAP for fetching identity information using LDAP.
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


Run SimpleSCIM with:

    LD_LIBRARY_PATH=/usr/local/lib SimpleSCIM file...

## Ubuntu 18.04
    sudo apt-get update && sudo apt-get upgrade
    sudo apt install git cmake pkg-config  libldap2-dev libcurl4-openssl-dev libboost-dev libsqlite3-dev uuid-dev
    git clone git://github.com/ola-mattsson/GroupSCIM.git
    cd GroupSCIM
    mkdir build
    cd build
    cmake ..
    make

## MacOS X 10.13
    Install Xcode
    Install HomeBrew
    brew install cmake openssl openldap
    git clone git://github.com/ola-mattsson/GroupSCIM.git
    cd GroupSCIM
    mkdir build
    cd build
    cmake ..
    make
    
## Windows
    Instructions not yet available, it's wip