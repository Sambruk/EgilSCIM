# Test server for EgilSCIM

Docs rudely ripped from adjacent `SimpleSCIM test server`

`test_server_node` is a simple server using openssl with the purpose
of testing the current set up of `EgilSCIM`. All `test_server_node` does
is send a HTTP response with the expected response code of a
successful SCIM transaction. When a user is created, a UUID is
generated and is returned as the only field in the returned JSON
object. `test_server_node` does not process the data received from the
client, so retrieving data from `test_server_node` is not possible. However
the scim messages are stored in a file by type in the 'out' folder adjacent
to the app folder. 

## Usage

### Set up

Set up `EgilSCIM` as described in docs int the docs folder

### Construct configuration file

Modify supplied configuration file examples as described in the doc folder

### Setup

```
Install nodejs
cd into test/test_server_node
npm install
mkdir out
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

### Test EgilSCIM

The 'out' folder contains all the scim messages sent to the test server. Each type as it's own
file, e.g. StudentGroup.log, SchoolUnit.log.
To get a live log of the out put as the client is run cd into the out folder and type for example:
```
tail -f StudentGroup.log
```
This prints the last 10 lines and waits for futher updates.
