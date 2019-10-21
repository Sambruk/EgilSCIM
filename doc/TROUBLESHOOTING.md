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
the client runs. It will show you the order in which LDAP queries are performed,
what the base and filter is for each query, and show the recursive process by
indenting the log file according to what is currently loaded.
