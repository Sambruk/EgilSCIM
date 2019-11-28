# Troubleshooting
There are a couple of methods that can be used to troubleshoot your
configuration.

# Verbose logging

By setting the configuration variable `verbose_logging` to `true`.

This gives additional diagnostic information when the program runs. Currently
this is only used for additional information about the HTTP traffic. It can
be useful to debug HTTP connection or authentication problems.

# HTTP log file

You can get all HTTP traffic to and from the client logged to a text file
by setting the variable `http-log-file` to a file path. If the file exists
it will be overwritten when the client runs.

# Load log file

You can log the load process to a text file by setting the variable
`load-log-file` to a file path. If the file exists it will be overwritten when
the client runs. It will show you the order in which objects are loaded
(regardless of whether they are loaded from LDAP or CSV), which LDAP queries
are performed, what the base and filter is for each query, and show the
recursive process by indenting the log file according to what is currently
loaded. Generated objects (Activity and Employment) are also included
with their UUIDs and information about which other objects they were generated
from.

# Forcing an object to be sent to the SCIM server

Sometimes you may wish to force the client to send an object to the SCIM server
even though it hasn't changed since it was last sent. There are two command
line arguments for this:

 * `--force-update`
 * `--force-create`

The object to be sent is specified by UUID. For instance:

```
$ EgilSCIMClient --force-update d80428c4-8788-47d7-aca7-761681fbe66a master.conf
```

Several UUIDs can be updated at once by specifying the argument multiple times:

```
$ EgilSCIMClient --force-update d80428c4-8788-47d7-aca7-761681fbe66a \
                 --force-update 75c666db-e60e-4687-bdd3-1af191fa6799 master.conf
```

Using `--force-update` will make the client behave as if the object in the cache file
differs from the object in the data source (e.g. LDAP).

Using `--force-create` will make the client behave as if the object didn't exist in
the cache file.

This will only work if the object already exists in both the cache file and in your
data source.
