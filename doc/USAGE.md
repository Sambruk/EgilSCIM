# Using SimpleSCIM

The intention is currently that for every remote service needing
user management, a configuration file is constructed. The
configuration file specifies how and where to fetch the user data
using LDAP and how and where to send the data using SCIM, as well as
how to relate the LDAP output to the SCIM requests.

## Configuration file

The formal grammar of a SimpleSCIM configuration file can be found in
the file `SimpleSCIM/res/config-file-grammar`. A configuration file
consists of a set of variable assignments. On each line, there can be
an optional variable assignment followed by an optional comment. A
variable assignment starts with a variable name followed by the `'='`
sign and a value. A variable name can be one or more of `'-'`, `'_'`,
`'a'`-`'z'`, `'A'`-`'Z'` and `'0'`-`'9'`. A value is either a single
line value or a multi line value. A single line value is terminated
by a comment or an end-of-line, and surrounding white space is
removed. A multi line value starts with `'<?'` and ends with `'?>'`.
Anything in between is the value, with no white space truncation
applied.

### Examples

```
var1 = 1 2 3            # var1 = "1 2 3"
var2 = <? 1 2 3 ?>      # var2 = " 1 2 3 "
var3 =                  # var3 = "" (empty value)
var4 = <?
    1 2 3       # Not a comment
?>                      # var4 = "\n    1 2 3       # Not a comment\n"
```

In these examples, different kinds of string values are assigned to
different variables. The value of `var1` is `"1 2 3"`, since it is a
single line value and the white space after the `'='` character and
before the `'#'` character is removed. The value of `var2` is
`" 1 2 3 "`, since it is a multi line value, even though it only
spans one line. Everything between `'<?'` and its matching `'?>'` is
the value, so the white space surrounding `"1 2 3"` is not removed.
The value of `var3` is `""`, the empty string, since it is a single
line value and between the `'='` character and the `'#'` character is
only white space, which is removed. The value of `var4` is
`"\n    1 2 3       # Not a comment\n"`, with `'\n'` representing the
end-of-line character, since it is a multi line value spanning 3
lines. Since the `"# Not a comment"` part is within the multi line
value, it is a part of the value and is not interpreted as a comment.

### Required variables

SimpleSCIM has a set of required variable names that must be assigned
meaningful values. The set of required variable names may change in
future versions of SimpleSCIM, but in the current version, the
required variable names are:

* `ldap-uri` is the uri to the LDAP server that contains the _schema_
  (i.e. `ldap://`, `ldaps://`, `ldapi://` and `cldap://`), the _host_
  and optionally the _port_ if a non standard port is being used,
  e.g. `ldaps://ldap.example.com:1234`.
* `ldap-who` is the DN to bind as.
* `ldap-passwd` is the password associated with the entry.
* `ldap-base` is the DN of the entry at which to start the search.
* `ldap-scope` is the scope of the search, i.e. `BASE`, `ONELEVEL`,
  `SUBTREE` or `CHILDREN`.


* `ldap-filter` is the filter to apply in the search. Leave empty for
  the filter `(objectClass=*)`.

* `cache-file` specifies the configuration file's cache file used to
  remember previous executions of the configuration file.
* `cert` is the path to the client's certificate file (PEM).
* `key` is the path to the client's private key file (PEM).
* `pinnedpubkey` is the server's hashed public key, e.g.
  `sha256//XYj98rkYBIYzCAc0NBYfooMUN38eq6xpQZOZP0b/jK8=`.
* `scim-url` specifies the protocol and host that will
  receive the SCIM request, e.g. `http://example.com/`.
* `scim-resource-identifier` specifies which variable in the returned
  JSON object that contains the SCIM resource identifier.
* `<type>-scim-json-templa` specifies the JSON object to send when creating and
    updating a new object.

### Template

The file `SimpleSCIM/res/template.conf` contains a template for a
SimpleSCIM configuration file that can be used as a basis for
creating other SimpleSCIM configuration files.

## Execution

SimpleSCIM is executed by typing `SimpleSCIM file...` where `file...`
is a list of zero or more SimpleSCIM configuration files. An example
of using this in a system is to add the command as a task to `cron`.

### Example

```
SimpleSCIM /etc/SimpleSCIM/conf/service{1,2}.conf
```

`/etc/SimpleSCIM/conf/service{1,2}.conf` expands to the path of both
`service1.conf` and `service2.conf`. First, the configuration file
for *service1* is executed to completion and then the configuration
file for *service2* is executed to completion.
