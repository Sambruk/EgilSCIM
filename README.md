# SimpleSCIM

SimpleSCIM is a program under development with the intended purpose
of demonstrating the usefulness of the SCIM protocol for
automatically managing user accounts in remote systems, and to be a
basic implementation of a SCIM client to be expanded upon.

## Intended usage

The intention is currently that for every remote service needing
user management, a configuration file is constructed. The
configuration file specifies how and where to fetch the user data
using LDAP and how and where to send the data using SCIM, as well as
how to relate the LDAP output to the SCIM requests.

### Configuration file

A configuration file has the following grammar:

```
<config>  ::= ( <ws>* <assign>? <comment>? '\n' )*
<comment> ::= '#' [^\n]*
<assign>  ::= <varid> <ws>* '=' <ws>* <value>
<varid>   ::= [-_a-zA-Z0-9]+
<value>   ::= '<?' [^('?>')]* '?>' <ws>*
            | [^('#'|'\n')]*                    # remove trailing <ws>*
<ws>      ::= ' ' | '\t'
```

A configuration file consists of a set of variable assignments. On
each line, there can be an optional variable assignment followed by
an optional comment. A variable assignment starts with a variable
name followed by the `'='` sign and a value. A variable name can be
one or more of `'-'`, `'_'`, `'a'`-`'z'`, `'A'`-`'Z'` and
`'0'`-`'9'`. A value is either a single line value or a multi line
value. A single line value is terminated by a comment or an
end-of-line, and surrounding white space is removed. A multi line
value starts with `'<?'` and ends with `'?>'`. Anything in between is
the value, with no white space truncation applied.

#### Examples

```
var1 = 1 2 3            # var1 = "1 2 3"
var2 = <? 1 2 3 ?>      # var2 = " 1 2 3 "
var3 =                  # var3 = "" (empty value)
var4 = <?
    1 2 3       # Not a comment
?>                      # var4 = "\n    1 2 3       # Not a comment\n"
```

#### Required variables

SimpleSCIM will have a set of required variable names that have not
yet been decided. They will be listed below.

* `ldapuri` contains the _schema_ (i.e. `ldap://`, `ldaps://`,
  `ldapi://` and `cldap://`), the _host_ (i.e. domain or IP address)
  and optionally the _port_ (i.e. `:port`) if it is not on a standard
  LDAP port.
* ...

### Execution

SimpleSCIM is executed by typing `SimpleSCIM file...` where `file...`
is a list of zero or more configuration files. An example of using
this in a system is to add the command as a task to `cron`.

#### Example

```
SimpleSCIM /etc/SimpleSCIM/conf/service{1,2}.conf
```

`/etc/SimpleSCIM/conf/service{1,2}.conf` expands to the path of both
`service1.conf` and `service2.conf`. First, the configuration file
for *service1* is executed to completion and then the configuration
file for *service2* is executed to completion.

## Compilation

SimpleSCIM currently requires the following C libraries:

* `glib-2.0` for the `GHashTable` data structure

SimpleSCIM plans to use the following libraries in the future:

* `libldap` from OpenLDAP for fetching identity information using LDAP
* `libcurl` to send the SCIM request

Compile the program by executing:

`make`
