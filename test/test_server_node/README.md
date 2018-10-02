# Test server for GroupSCIM

Docs rudely ripped from adjacent `test_server`

`test_server_node` is a very simple server using openssl with the purpose
of testing the current set up of `GroupSCIM`. All `test_server_node` does
is send a HTTP response with the expected response code of a
successful SCIM transaction. When a user is created, a UUID is
generated and is returned as the only field in the returned JSON
object. `test_server_node` does not save the data received from the
client, so retrieving data from `test_server_node` is not possible.

## Usage

### Set up

Set up `GroupSCIM` as described in docs int the docs folder

### Construct configuration file

Modify supplied configuration file examples as described in the doc folder

### Setup

```
Install nodejs
cd into test/test_server_node
npm install
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
pinnedpubkey = sha256//hash
```
where `hash` is the output, for example:

```
pinnedpubkey = sha256//wody0BSEtvWoP+DX5KRuCUjq4uSTRvXLOe6vjehxWNY=
```

### Start the server

open test/test_server_node/app/config.js in your favorite text editor and enter file names of
the new certificates, and the port you enter in to the configurations files
```
    port: 9876,
    httpsOptions: {
        key: fs.readFileSync("<path>/some-key.pem"),
        cert: fs.readFileSync("<path>/some-cert.pem")
    },
```
Inside test/test_server_node type
```
npm start
```

Ensure that the configuration file has these lines:

```
scim-url = https://localhost:<port>/Users
scim-resource-identifier = id
```

where `<port>` is the port entered when starting the server.

### Test GroupSCIM

Running `GroupSCIM` with the configuration file should now cause the
server to print the incoming requests from `GroupSCIM` without
processing them at all. Running `GroupSCIM` again with the same
configuration file should only report users being copied, meaning no
requests are sent to `test_server`. If any attribute or user in the
data source is altered, added or removed, only these changes will be
sent the next time `GroupSCIM` is executed with the same
configuration file. To inspect the content of the cache, run these
commands:

```
cd /path/to/GroupSCIM/src
make simplescim_cache_file_print
./simplescim_cache_file_print cache-file
```

where `cache-file` is the cache file specified in the configuration
file. The cache file is only created if a user is successfully sent
to the server.
