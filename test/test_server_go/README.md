# Test server for EgilSCIM

`test_server_node` is a simple server using openssl with the purpose
of testing the current set up of `EgilSCIM`. It is not a complete SCIM
server, it simply receives and logs the requests done by the client
and returns successful status codes back so the client thinks everything
is accepted.

The test server can also be used as the driver for the automated test
suite.

## Usage

### Building the test server

To build the test server the toolchain for the Go programming language
must be installed.

For instance, on Linux Debian or Ubuntu:
```
apt-get install golang
```

Then go into the directory EGILTestServer and build the program:

```
cd test/test_server_go/EGILTestServer
go build
```

You should now have an executable binary named EGILTestServer in
that directory.

#### Building using Nix
```
cd <project-root>
nix build '.#egil-test-server'
```

### Generate server certificate and private key

If you don't already have a certificate and private key which the test server
can use, you can generate a self signed certificate like this:

```
openssl req -x509 -newkey rsa:4096 -keyout test-key.pem -out test-cert.pem -days 365 -nodes
```

### Hash the servers public key for pinning

You can create a hash from the public key with a script in the tools directory:

```
tools/public_key_pin.sh test-key.pem
```

write the output to the client's configuration file as:

```
pinnedpubkey = sha256//hash
```
where `hash` is the output, for example:

```
pinnedpubkey = sha256//wody0BSEtvWoP+DX5KRuCUjq4uSTRvXLOe6vjehxWNY=
```

### Start the server

To start the server (without running the test suite), specify the certificate
and key on the command line:

```
./EGILTestServer -cert test-cert.pem -key test-key.pem
```

### Test EgilSCIM

The test server listens on port 8000. So configure the EGIL client to connect
to  localhost port 8000:

```
scim-url = https://localhost:8000
```

When the client sends requests to the test server, it will log those requests
in .log files, one file per resource type (e.g. StudentGroups.log and
SchoolUnits.log).

If there were existing log files when the server started, they will be
truncated.

## Running the test suite
The test suite can be started by running the
[run_test_suite](../scripts/run_test_suite) file from any directory.

### Using Nix
```shell
cd <project-root>
nix run '.#egil-test-suite`
```
