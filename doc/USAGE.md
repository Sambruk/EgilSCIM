# Using EgilSCIM

The intention is currently that for every remote service needing
user management, a configuration file is constructed. The
configuration file specifies how and where to fetch the user data
using LDAP and how and where to send the data using SCIM, as well as
how to relate the LDAP output to the SCIM requests.

## Configuration file

The formal grammar of an EgilSCIM configuration file can be found in
the file `EgilSCIM/res/config-file-grammar`. A configuration file
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

### Variables

EgilSCIM has a set of required variable names that must be assigned
meaningful values. The set of required variable names may change in
future versions of EgilSCIM and depends on whether LDAP and/or CSV
is used as data source.

The following variables should be configured regardless of data source:

* `cache-file` specifies the configuration file's cache file used to
  remember previous executions of the configuration file.
* `cert` is the path to the client's certificate file (PEM).
* `key` is the path to the client's private key file (PEM).
* `scim-type-send-order` is the order in which objects are sent to the SCIM server.
* `scim-type-load-order` is the order in which we load objects from the data source.

Type specific configuration can be placed in a separate configuration file
(e.g. Student.conf). To include such a configuration file, the main configuration
file needs to specify it in `<type>-scim-conf`, for instance:

```
Student-scim-conf = Student.conf
```

Type-specific variables:

* `<type>-scim-url-endpoint` specifies where to send objects of this type.
* `<type>-unique-identifier` is the name of the attribute which should be used as UUID.
* `<type>-hidden-attributes` can be used to specify attributes which should be fetched from LDAP even though EgilSCIM can't deduce it automatically (for instance because they're not used in the JSON templates).
* `<type>-remote-relations` is used to define relations between objects.
* `<type>-scim-json-template` specifies the JSON object to send when creating and
    updating a new object.

#### LDAP connection and authentication

The following variables need to be configured in order to connect to
an LDAP server:

* `ldap-uri` is the uri to the LDAP server that contains the _schema_
  (i.e. `ldap://`, `ldaps://`, `ldapi://` and `cldap://`), the _host_
  and optionally the _port_ if a non standard port is being used,
  e.g. `ldaps://ldap.example.com:1234`.
* `ldap-who` is the DN to bind as.
* `ldap-passwd` is the password associated with the entry.

#### SCIM connection and authentication
If you wish to run against Skolfederation's metadata file, the following
set of variables need to be configured:

* `metadata-path` is the full path to the metadata file
* `metadata-entity` is service provider's entity id in the metadata

You can also manually specify the information which would otherwise be
fetched from metadata. This can be useful for test purposes or if the
service provider for some reason isn't included in the metadata. In this case
the following variables need to be configured:

* `scim-url` specifies the base URL for the SCIM end-point, e.g. `https://scim.serviceprovider.se/scim/v2`.
* `pinnedpubkey` is the server's hashed public key, e.g.
  `sha256//XYj98rkYBIYzCAc0NBYfooMUN38eq6xpQZOZP0b/jK8=`.
* `metadata_ca_path` is the path to the directory containing the certificate store.
* `metadata_ca_store` is the name of a CA store in PEM file format.

## Loading objects from LDAP

The objects will be loaded in the order specified in the variable `scim-type-load-order`.
During the loading process, objects related to those loaded will also be loaded recursively.
For instance, if StudentGroup objects are loaded first and those objects have relations to
Student objects, those Student objects will also be loaded as a consequence of loading the
StudentGroups. Because of this, `scim-type-load-order` should not include those object
types that should be found through relations.

For every type _X_ included in `scim-type-load-order` there should either be a variable named
`X-ldap-filter`, which tells the client how to search for the objects in the LDAP server, or
the type needs to be one of the generated types (Employment and Activity).

Since the generated types are generated based on the objects loaded from LDAP, the generated
types should come last in `scim-type-load-order`.

## Loading objects from CSV

If objects of a type _X_ should be loaded from CSV files instead of from LDAP, they should
have a `X-csv-files` variable instead of the `X-ldap-filter`.

The `X-csv-files` variable should be a path to a CSV file, or a space separated list of paths.
The first file is used to load all objects of that type, one object per record in the CSV file.
The columns in the file will correspond to attributes in the objects created.

If more than one file is specified, the other files are used to specify multi-valued
attributes. These files should have two columns, the first acting as a foreign key to the
objects given in the first file and the second containing values for the attribute.

### Example

Lets say we have groups in CSV files, and each group has a multi-valued "members" attribute.

In your EgilSCIM configuration file, specify that StudentGroup should be loaded from CSV:

```
StudentGroup-csv-files = groups.csv members.csv
```

groups.csv might then look something like this:

```
groupName,schoolUnitCode
English-XC-03,12345678
History-CC-02,12345678
```

This alone would give us groups with two single-valued attributes. The multi-valued "members"
attribute comes from the members.csv file, which might look something like this:

```
groupName,member
English-XC-03,student2
History-CC-02,student1
English-XC-03,student1
```

The first column name specifies which attribute in the first file to use to identify the object
(make sure it can be used as a unique identifier). The second column name gives the name of
the multi-valued attribute in the object.

### UUIDs

In the example above, there are no UUIDs given. These can be included as its own column if you
have appropriate UUIDs, or they can be generated by EgilSCIM on the fly based on one of the
other attributes. This is done with the `<type>-UUID-generator` variable, for example:

```
StudentGroup-UUID-generator = groupName
```

Make sure that the attribute used really is something that uniquely identifies the object
(not just unique within that type of objects, but within all types). The generated UUID
attribute will be available as whatever you've specified as `<type>-unique-identifier`.

### CSV format

The files are assumed to conform to [RFC 4180](https://tools.ietf.org/html/rfc4180) with
the following additions:

 * The header line must be included
 * Unix-style line endings are allowed
 * The separator character can be configured (with the `csv-separator` variable)
 * The quote character can be configured (with the `csv-quote` variable)
 * UTF-8 encoded text is allowed

## Defining relations between objects

Objects can be related to other objects. For instance a StudentGroup is typically
related to the Students and Teachers that are included in the group. A
StudentGroup is also typically associated with a single SchoolUnit object.

The relations for a type _X_ is defined in the variable _X_-remote-relations.
For instance, for a StudentGroup it might look something like:

```
StudentGroup-remote-relations = <?
{
    "relations": {
        "SchoolUnit": {
            "local_attribute": "owningSchoolUnit",
            "remote_attribute": "schoolUnitCode",
            "ldap_base": "ou=SchoolObjects,o=Organisation",
            "ldap_filter": "(schoolUnitCode=${value})",
            "method": "ldap"
        },
        "Student": {
            "local_attribute": "groupMember",
            "remote_attribute": "personFDN",
            "ldap_base": "${value}",
            "ldap_filter": "(roleIdentifier=Student)",
            "method": "ldap"
        },
        "Teacher": {
            "local_attribute": "groupMember",
            "remote_attribute": "personFDN",
            "ldap_base": "${value}",
            "ldap_filter": "(roleIdentifier=Staff)",
            "method": "ldap"
        }
    }
}
?>
```

The relations are defined in JSON format in an object called `relations`. The `relations`
object will contain one member for each other type with which we want to establish a relation.
In the example above, StudentGroup will have relations to SchoolUnit, Student and Teacher.

### LDAP relations
If the `method` attribute in the relation is set to "ldap", the related object will be found
by executing LDAP queries. Typically there is an LDAP attribute in one of the objects that should
be matched with an LDAP attribute in the other object. The `local_attribute` should be set to
the name of the LDAP attribute of the type for which we're defining the relations. In the
example above where we're defining relations for StudentGroups, `local_attribute` should
specify an LDAP attribute for the student groups. `remote_attribute` is the corresponding
attribute in the related object. The `local_attribute` can be a single-valued or multi-valued
attribute, for each value there will be an LDAP query executed with whatever is specified in
`ldap_base` and `ldap_filter`. If the special variable `${value}` is used anywhere in `ldap_base`
or `ldap_filter`, that will be replaced by the value from the local attribute.

The LDAP query should return zero (no match) or exactly one object (which will then be related).

If an object is found this way, and it hasn't already been loaded, the load process will
continue recursively from that object and load its related objects as well.

### Object relations
If the `method` attribute in the relation is set to "object", the related object will be
found by searching through the objects which have already been loaded into memory.

In this case, `ldap_base` and `ldap_filter` need not be specified and the related object
is found simply by matching `local_attribute` against `remote_attribute`.

In order to use this method, the related objects' type need to be listed before the type
from which we wish to establish the relation in `scim-type-load-order`. For instance, if
the object method is used in `StudentGroup-remote-relations` to find the SchoolUnit object
for a given StudentGroup, SchoolUnit should come before StudentGroup in `scim-type-load-order`,
so that the SchoolUnit objects are already loaded in memory when we load the StudentGroups.

## Limiting the load process

Which objects to load from LDAP is typically specified with an LDAP filter as described above.
However, this can be further restricted in other ways. This can be helpful if you want to
specify a long list of specific objects, or if you have one shared configuration file which loads
everything from LDAP but which needs to be restricted in different ways depending on the recipient
(SCIM server).

### File of attribute values

If you'd like to specify exactly which objects to load you can create a text file containing
a list of values. You can choose which type to limit and by which attribute. For instance, if
you'd like to specify a list of StudentGroups to include:

```
StudentGroup-limit-with = list
StudentGroup-limit-list = groups.txt
StudentGroup-limit-by = cn
```

The first variable specifies that we want to use a file with a list of values. The second variable
gives the path to that file. The third variable specifies which LDAP attribute to match against
the values in the list.

Only those objects which match one of the values in the file will be included in the load process.

If you want to match against UUID the `limit-by` variable should be omitted.

## Execution

EgilSCIM is executed by typing `EgilSCIMClient [OPTIONS] <file>` where `file`
is an EgilSCIM configuration file. Most options can be set either
as a command line argument or in the configuration file. For a list
of options, run the program without arguments.

### Example

```
EgilSCIMClient /etc/EgilSCIM/conf/service1.conf
```

If an option is set in both the configuration file and on the command line,
the command line argument takes precedence. This way you can reuse the same
configuration file for different service providers.

```
EgilSCIMClient --metadata-entity https://service1.com --cache-file /etc/EgilSCIM/cache/service1 /etc/EgilSCIM/conf/standard.conf
```

## Cache file

After an initial sync has been done to the SCIM server, we would ideally
only send changed objects. The client stores all the objects it sends to
the server in its cache file, when the client runs next time it will compare
the current data against the cache file and figure out which objects need
to be updated, created or deleted.

It's important that the cache file is stored somewhere safe, and that
different cache files are used for different SCIM servers.

### Rebuilding the cache file

If all works as it should you shouldn't need to rebuild the cache file.
But if the cache file is lost or corrupted, or in some cases when the
server and client are out of sync, you can ask the client to rebuild
the cache file.

Note that this will only work if the SCIM server has implemented querying
the resource type endpoints so that the client can fetch all objects.

The server does not need to implement filtering or sorting, but it needs
to be able to return all objects for a resource type (either as one response
or with pagination).

The client will rebuild the cache by first fetching all UUIDs from the SCIM
server. Objects that exist both locally (e.g. in LDAP) and in the SCIM server
will always be updated (i.e. sent to the SCIM server again). In other words,
no comparison is done. The whole process can take a long time, similar to the
first sync without a cache.

To rebuild the cache, run the client with the `--rebuild-cache` argument.

You should still specify a path to a cache file (either on the command line
or in the config file), so a new cache file can be created. Next time you
run the client there shouldn't be a need for `--rebuild-cache` anymore.
