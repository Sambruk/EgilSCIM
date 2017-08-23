# Test server for SimpleSCIM

`test_server` is a very simple server using openssl with the purpose
of testing the current set up of `SimpleSCIM`. All `test_server` does
is send a HTTP response with the expected response code of a
successful SCIM transaction. When a user is created, a UUID is
generated and is returned as the only field in the returned JSON
object. `test_server` does not save the data received from the
client, so retrieving data from `test_server` is not possible.

## Usage

### Set up

Set up `SimpleSCIM` as described in
<https://github.com/MaxWallstedt/SimpleSCIM/wiki>.

### Construct configuration file

Construct configuration file a as described in
<https://github.com/MaxWallstedt/SimpleSCIM/blob/master/README.md>
and
<https://github.com/MaxWallstedt/SimpleSCIM/blob/master/JSON-REPLACEMENT-RULES.md>

### Compile

`test_server` uses openssl and libuuid. On Fedora 26, libuuid can be
installed with `sudo dnf install libuuid-devel`.

```
cd /path/to/SimpleSCIM/test_server
gcc -g -pedantic -Wall -Wextra -Werror -o test_server test_server.c -lssl -lcrypto -luuid

```

### Generate server certificate and private key

```
openssl req -x509 -newkey rsa:4096 -keyout test-key.pem -out test-cert.pem -days 365 -nodes
```

### Hash the servers public key for pinning

```
openssl x509 -in test-cert.pem -pubkey -noout | openssl pkey -pubin -outform der | openssl dgst -sha256 -binary | openssl enc -base64
```

write the output to the configuration file as:

```
...
pinnedpubkey = sha256//hash
...
```

where `hash` is the output, for example:

```
...
pinnedpubkey = sha256//wody0BSEtvWoP+DX5KRuCUjq4uSTRvXLOe6vjehxWNY=
...
```

### Start the server

```
./test_server port test-cert.pem test-key.pem
```

Ensure that the configuration file has these lines:

```
scim-url = https://localhost:port/Users
scim-resource-identifier = id
```

where `port` is the port entered when starting the server.

### Test SimpleSCIM

Running `SimpleSCIM` with the configuration file should now cause the
server to print the incoming requests from `SimpleSCIM` without
processing them at all. Running `SimpleSCIM` again with the same
configuration file should only report users being copied, meaning no
requests are sent to `test_server`. If any attribute or user in the
data source is altered, added or removed, only these changes will be
sent the next time `SimpleSCIM` is executed with the same
configuration file. To inspect the content of the cache, run these
commands:

```
cd /path/to/SimpleSCIM/src
make simplescim_cache_file_print
./simplescim_cache_file_print cache-file
```

where `cache-file` is the cache file specified in the configuration
file. The cache file is only created if a user is successfully sent
to the server.
