# Debugging EgilSCIM

In order to find the root cause of a crash bug, or to run the program
successfully in a debugger, the program should be built in Debug-mode.

To do this, follow the standard instructions in [INSTALL](INSTALL.md)
but replace

```
cmake ..
```

with

```
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

## Post-mortem investigation of a crash

To do a post-mortem investigation of a crash, we need the following:

 * The EgilSCIMClient binary
 * The source code (same version as was used when building the binary)
 * The core dump file from the crash

For some systems, generation of core dumps is disabled. For information on
how to enable it, or where to find the generated core dumps, refer to
documentation for your operating system.

You can then start a debugging session in e.g. gdb:

```
gdb EgilSCIMClient core
```
