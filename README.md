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

SimpleSCIM has a set of required variable names that are not yet
decided. The following variable names are suggested required
variables:

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
* `ldap-attrs` is a comma separated list of attribute descriptions to
  return from matching entries. Leave empty to return all attributes.
* `ldap-attrsonly` is a boolean variable (i.e. `TRUE` or `FALSE`)
  that should be set to `TRUE` if only attribute descriptions are
  wanted. It should be set to `FALSE` if both attribute descriptions
  and attribute values are wanted.
* `user-unique-identifier` is the attribute that uniquely identifies
  a user.
* `user-scim-resource-identifier` is the attribute that will contain
  the SCIM resource identifier in the cache.
* `cache-file` specifies the configuration file's cache file used to
  remember previous executions of the configuration file.
* `cert` is the path to the client's certificate file (PEM).
* `key` is the path to the client's private key file (PEM).
* `pinnedpubkey` is the server's hashed public key, e.g.
  `sha256//XYj98rkYBIYzCAc0NBYfooMUN38eq6xpQZOZP0b/jK8=`.
* `scim-url` specifies the protocol, host and resource that will
  receive the SCIM request, e.g. `http://example.com/User`.
* `scim-resource-identifier` specifies which variable in the returned
  JSON object that contains the SCIM resource identifier.
* `scim-create` specifies the JSON object to send when creating a new
  object.
* `scim-update` specifies the JSON object to send when updating a
   remote object.

##### Template

`/etc/SimpleSCIM/conf/test.conf`:

```
# LDAP variables

ldap-uri       = ldaps://ldap.example.com
ldap-who       = # -D in ldapsearch
ldap-passwd    = # -w in ldapsearch
ldap-base      = # -b in ldapsearch
ldap-scope     = # BASE, ONELEVEL, SUBTREE or CHILDREN
ldap-filter    = # LDAP filter
ldap-attrs     = # Empty => All attributes
ldap-attrsonly = FALSE

# User variables

user-unique-identifier = uid
user-scim-resource-identifier = scim-id

# Cache path

cache-file = /etc/SimpleSCIM/cache/test.cache

# Certificate variables

cert = /etc/SimpleSCIM/cert/cert.pem
key = /etc/SimpleSCIM/cert/key.pem
pinnedpubkey = sha256//XYj98rkYBIYzCAc0NBYfooMUN38eq6xpQZOZP0b/jK8=

# SCIM variables

scim-url = https://scim.example.com/Users
scim-resource-identifier = id

scim-create = <?
{
 "schemas":["urn:ietf:params:scim:schemas:core:2.0:User"],
 "userName":"${uid}",
 "externalId":"${uid}",
 "name":{
  "formatted":"${fullName}",
  "givenName":"${givenName}"
 },
 "emails":[
  ${for $e in email}
  {
   "value":"${$e}"
  },
  ${end}
 ]
}
?>

scim-update = <?
{
 "schemas":["urn:ietf:params:scim:schemas:core:2.0:User"],
 "id":"${scim-id}",
 "userName":"${uid}",
 "externalId":"${uid}",
 "name":{
  "formatted":"${fullName}",
  "givenName":"${givenName}"
 },
 "emails":[
  ${for $e in email}
  {
   "value":"${$e}"
  },
  ${end}
 ]
}
?>
```

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

* `uthash` (included in repo for now) for hash tables.
* `libldap` from OpenLDAP for fetching identity information using LDAP.
* `json-c` to parse the JSON objects from the configuration file.
* `libcurl` to send the SCIM request.

Compile the program by executing:

`make`
