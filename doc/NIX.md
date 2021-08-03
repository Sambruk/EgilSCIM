# Nix
At the root of the repository there is a [flake.nix](../flake.nix) file that
defines nix expressions to build various programs/tools/scripts that is used
inside the repository, as well as a VM test and a development shell. All of
these can be used with the [Nix package manager](https://nixos.org/) without
needing any other dependency.

## Supported systems
The flake is usable for systems included in the `defaultSystems`[^1] defined by
the "flake-utils" library. Which includes at the time of writing:

### Linux
- aarch64
- i686
- x86_64

### Darwin
- aarch64
- x86_64

[^1]: https://github.com/numtide/flake-utils/blob/master/default.nix#L3

## Packages
- `plugins/echo`: An example plugin for EgilSCIM.
- `scim-client`: The main application of the repository.
- `scim-client.debug`: The same as `scim-client` but built with debug flags
  enabled.
- `test/server`: The test server that can bu used with the test suite. See
  [the test server README](../test/test_server_go/README.md) for more
  information.
- `test/suite`: The system test suite for EgilSCIM.
- `tools`: An umbrella package that contains the other tools. See [the Egil
  tools README](../tools/README.md) for more information.
- `tools/fetch_metadata`
- `tools/list_metadata`
- `tools/public_key_pin`

## Hydra VM test
A VM test for the system test suite is defined to be run by an Hydra instance
but it could also be run manually with:
```shell
nix build '.#hydraJobs.vmTest.egil-test-suite.x86_64-linux'
```
Note that the VM test is only for "x86_64-linux" systems; partly because NixOS
test driver only works for Linux OS and partly because the Docker image used by
the test suite is only available for the "x86_64" architecture.

## devShell
There is a `devShell` defined which can be used with `nix develop` to start an
development shell with the build environment of `scim-client.debug`. On top of
the dependencies of `scim-client.debug`, `gdb` and `egil/test/suite` is also
usable within the shell.

So you can run the test suite by just typing `run_test_suite` in the shell
without needing to install `docker` cli or `openssl`. One thing that is required
is the docker daemon, which is not possible to install with a nix shell as far
as I know.

On the other hand, you can start debugging the EgilSCIM with just typing `gdb
EgilSCIMClient`, which should start the gdb with the external debug
symbols of the dependencies already loaded if they exist. The source code
directory will also be correctly found by the gdb, regardless of the location
that it is started from.
