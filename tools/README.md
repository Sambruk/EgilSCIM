# Tools associated with the EGIL client

This directory is for tools that may simplify usage of the
EGIL client.

## fetch_metadata
The script fetch_metadata.py will both download and verify the authentication
metadata against a key. The decoded metadata can then be used by the EGIL
client in order to connect to and authenticate a server.

Before running the script you need to make sure that Python 3 and the jwcrypto
package is installed.

For instance, to install on Debian these commands can be used:

```
sudo apt-get install python3 python3-pip
sudo pip3 install jwcrypto
```

(If you don't wish to install jwcrypto globally, or don't have root access,
you can use ```virtualenv``` or ```pip3 --user```)
