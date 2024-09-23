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
future versions of EgilSCIM and depends on whether LDAP, CSV and/or
SQL is used as data source.

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
* `<type>-readable-id` can be used to specify an attribute to use when referring to an object in error messages or log files (otherwise only the UUID will be used)

#### LDAP connection and authentication

The following variables need to be configured in order to connect to
an LDAP server:

* `ldap-uri` is the uri to the LDAP server that contains the _schema_
  (i.e. `ldap://`, `ldaps://`, `ldapi://` and `cldap://`), the _host_
  and optionally the _port_ if a non standard port is being used,
  e.g. `ldaps://ldap.example.com:1234`.
* `ldap-who` is the DN to bind as.
* `ldap-passwd` is the password associated with the entry.

You may also choose not to follow LDAP referrals:

```
ldap-follow-referrals = false
```

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

If you wish to use the tab character as a separator character it needs to be
set with a special escape sequence:

```
csv-separator = \t
```

## Loading objects from SQL

If your source data is in a relational database, loading directly from SQL instead
of exporting to CSV first offers some advantages:

 * There's no need to automate CSV exports and syncronize that with EGIL jobs
 * You can easily filter what data to include in the EGIL configuration files
 * Attributes can be optional (by using NULL values in the database)

In order to use SQL as a data source, a plugin suitable for your database needs to be
available. There are no plugins included in the open source version of the EGIL client.
If you wish to develop your own plugin, see the required C interface of the plugin in
`src/sql_interface.h`.

To specify a plugin to use, assuming you have the `odbc` plugin installed in the directory
`/usr/lib/egil/sql`, add the following variables to your master configuration file:

```
sql-plugin-path = /usr/lib/egil/sql
sql-plugin-name = odbc
```

The plugin may also have its own configuration variables, starting with `sql` and the name of
the plugin, for instance to specify a connection string to the `odbc` plugin:

```
sql-odbc-connection-string = Driver={PostgreSQL Unicode};Database=egil;
```

Loading objects from SQL is similar to loading from CSV files. Instead of specifying
filenames, SQL queries are used to fetch tables from the database. For each object
type there's a main SQL query that is used to fetch attributes for the objects. If
there are multi-valued attributes auxilliary queries can be specified, where each
such query should return a table with two columns. Just like for CSV files, the first
column works as a foreign key to the main table and should have the same name as an
attribute that acts as a primary key in that table. The second column contains the
values.

### Simple example

Like the LDAP queries, SQL queries are specified in the configuration file in JSON format.
The easiest example, for just loading single-valued attributes, looks like this:

```
Organisation-sql = <?
{
  "query": "SELECT * FROM organisation"
}
?>
```

### Multi-valued example

Here's an example for loading teachers, where each teacher can have several employments:

```
Teacher-sql = <?
{
  "query": "SELECT * FROM teachers",
  "aux": [
  	{ "query" : "SELECT uid, schoolunitcode FROM employments" }
  ]
}
?>
```

In this case, the employments table in the database has a uid column which is used
as a foreign key in order to link the data to the teachers table. The loaded
teacher objects will get attributes named `schoolunitcode` which may have multiple
values.

Note that just like for CSV files, the EGIL client will join the tables in memory.
However it might still be a good idea to include an SQL JOIN in your queries for
the auxilliary tables so as to not include attributes for objects not included in
the main table. It should be more efficient to let the database reduce the number
of rows as much as possible before handing them over to the EGIL client.

If the foreign key in the table with multi-valued attributes isn't named exactly
like the primary key in the main table, you can use "AS" in your SQL query to
return a table with the proper column name. For instance:

```
SELECT id AS uid, email FROM emails
```

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

### Required relations
Sometimes an object needs to have a relation to another object. For instance, a StudentGroup
in SS12000 needs to have an owning SchoolUnit. If there are groups in the data source
without a school unit attribute, or with a school unit attribute that points to a non-existing
school unit, we can specify that these groups should be skipped in the load process by
specifying that the relation is required:

```
  "require": "true",
``` 

By default, the object that fails to establish a relation will be silently skipped. If
you don't expect this to happen you might prefer to get a warning. See the section below
about referential integrity warnings.

### Referential integrity warnings
Sometimes when the value for the local attribute doesn't find a matching remote attribute
we want to know about it. For instance, perhaps you expect all groups to have valid
school unit codes. You can then specify that the relation should warn about attribute
values that don't find a match:

```
  "warn_missing": "true",
```

After trying to establish relations for a data type, you will get a summary of all
local attribute values for which there was no match (up to 100 values).

## Generating groups from attributes

Sometimes the student groups are not available as their own objects in the data source.
Instead they're available as membership attributes for the users. For instance students
might have a multi-valued attribute such as "groups" which lists all groups the user
is a member of (one value per group).

If these attributes contain enough information about the groups the client can generate
"virtual" groups based on these attributes. For EGIL that typically means that the attribute
values need to at least include the groups name and school unit (usually a school unit code).
The values also need to be in a format that can be parsed with regular expressions.

For instance, if users have group attributes with values such as "12345678#Math 1A", we
can create a virtual group with display name "Math 1A" and school unit code "12345678".

The virtual group will get a generated UUID based on the parts of the attribute values
that uniquely identify the group.

To use virtual groups, make sure the users from which the groups should be generated
are loaded before the groups in the scim-type-load-order variable.

Then make sure the group type is a generated type, for instance:

```
StudentGroup-is-generated = true
```

Then specify from which user types' attributes to generate the groups, for instance if
both students and teachers have these membership attributes:

```
StudentGroup-generate-from-types = Teacher Student
```

Finally, specify how to generate groups from the attributes. Here's an example:

```
StudentGroup-generate-from-attributes = <?
[
  {
    "from": "classes",
    "match": "(.*)#(.*)",
    "uuid": "$1$2",
    "attributes": [ [ "studentGroupType", "Klass" ], [ "schoolUnitCode", "$1" ], [ "displayName", "$2" ] ]
  },
  {
    "from": "educationGroups",
    "match": "(.*)#(.*)",
    "uuid": "$1$2",
    "attributes": [ [ "studentGroupType", "Undervisning" ], [ "schoolUnitCode", "$1" ], [ "displayName", "$2" ] ]
  }
]
?>
```

In this example we're generating groups from two attributes, `classes` and `educationGroups` (see the
`from` attribute for each).

For each attribute we specify a regular expression (`match`). Attribute
values which do not match the regular expression are simply ignored. Note that capture groups are
used in the regular expressions (the parentheses in the regular expressions). In this case the first
capture group matches the school unit code, and the second matches the group's display name. We
can now refer to these individual parts with `$1` (school unit code) and `$2` (group display name).

Then we need to specify how to generate a unique id for the generated groups (`uuid`). In this
case `$1` and `$2` will uniquely identify a group, so we use those two concatenated when we
generated a UUID for the groups.

Then we specify which attributes the virtual group should have. We use `$1` and `$2` to give the
generated group a schoolUnitCode attribute and a displayName attribute. We also give the group
a studentGroupType attribute, with hard coded values depending on which attribute was used
from the user (so groups generated from a user's `classes` attribute will get `studentGroupType`
set to "Klass", and groups generated from a user's `educationGroups` attribute will get
`studentGroupType` set to "Undervisning").

## Generated objects (Employment and Activity)

The SS12000 data types Employment and Activity often don't have corresponding objects in the
data source. For employments this is often simply modelled as an attribute on the user accounts
(for instance a multi valued attribute containing school unit codes for the user's employments),
not as stand-alone objects.

The Activity objects are (within EGIL) only used to associate teachers with student groups, and
this relation is often included in the group's membership list rather than as separate Activity
objects in the data source.

Because of this the EGIL SCIM client can be configured to generate these objects based on the
other objects (Employments are generated from the Teachers and SchoolUnits, Activity objects
are generated from StudentGroups and Teachers).

There are commented examples for both Activity and Employment in the standard example in the
`master_config` directory.

### Associating Activity objects with national test activities
When provisioning to the Swedish National Agency for Education (Skolverket) for the
national assessments (DNP) student groups of type "Undervisning" should be associated
with one of the test activities published by Skolverket. This is done in the group's
Activity object by specifying a parent activity which points to the test activity's UUID.

Usually the data sources don't contain these UUIDs so the EGIL SCIM client has built-in
support for converting from the test activities' names to UUIDs. A name such as
"GRGRMAT01_6" can then be converted to "b229977a-7bd3-58ad-b7a2-3fdc774840fd".

The available test activities are published in Skolverket's open data API. To do the
conversion automatically the URL to the activities API need to be configured, e.g.:

```
Activity-national-test-activities-url = https://apigw-pre.skolverket.se/provtjanst/verifieringstest/open-data/v1/activities
```

If you prefer to have the file downloaded locally you can specify a file:// URL:

```
Activity-national-test-activities-url = file:///workdir/data/activities.json
```

You also need to specify which attribute in the student group contains the test activity name:

```
Activity-national-test-activity-name-attribute = testName
```

With the settings above the EGIL SCIM client will, when generating an Activity object, read the
student group's `testName` value and convert it to a UUID according to the test activities read
from the URL. The UUID will be placed in a new attribute in the Activity object, which by default
will be named `parentActivity`. The `parentActivity` attribute will only be created if the test
name can be matched with a test activity in the data read from the URL. If you wish to use a
different attribute name for the UUID you can specify it:

```
Activity-national-test-activity-id-attribute = testUUID
```

But there is usually no need to do so. Assuming you use the default attribute (`parentActivity`)
it can then be specified like so in the Activity object's JSON template:

```
    <...>
    
    ${for $id in parentActivity}
    "parentActivity": [
      {
        "value": "${$id}"
      }
    ],
    ${end}
    <...>
```

In many cases the data source doesn't already have an attribute for test name in the student
groups. If the test name (or something that can be mapped to the test name) is available as a
part of the group's name you will need to configure a transform so as to create a new attribute
in the group which contains the test name.

In many cases the information that could be mapped to a test name will only map to a subject
or course code. For instance you may have the text "-MA1-" as a substring in the names of all
groups that correspond to the subject "GRGRMAT01", but can't get the full test name including
the suffix (e.g. "GRGRMAT01_6"). In this case you we can try to deduce it.

The suffix is either a school year or a school type, so we need to configure where to find
those values. If possible we should read both from the StudentGroup object:

```
Activity-deduce-test-activity-suffix-from-school-type-attribute = schoolType
Activity-deduce-test-activity-suffix-from-school-year-attribute = schoolYear
```

If the StudentGroup doesn't have an attribute for schoolType we can sometimes take it from the
SchoolUnit instead:

```
Activity-deduce-test-activity-suffix-from-school-type-attribute = SchoolUnit.schoolType
```

which only works if we can safely assume that each SchoolUnit only offers one school type.

If the StudentGroup doesn't have an attribute for schoolYear we can take it from the Students
by specifying how to find the Students from the StudentGroup object:

```
Activity-deduce-test-activity-suffix-from-members-with-school-year = Student.entryUUID
```

In other words we point out the student's UUIDs. Note that if this method is used to find the
school year, the attribute `Activity-deduce-test-activity-suffix-from-school-year-attribute`
should specify an attribute in the Student objects. If the students have different school years
we'll use the most common value.

### Overriding Employment attributes

For the generated Employment objects it is possible to override attributes with a separate
data source (such as a CSV file or an SQL table). The typical use case for this is for
employment roles (teacher/principal etc.).

Since Employments are generated from the staff's user objects in data sources where there
aren't stand-alone employment objects, it can be difficult to model attributes for the
employments correctly. For instance, employment role is typically a single-valued attribute
on the user object. This works for most users, but can lead to some problems for users with
multiple employments with different roles.

If it's not reasonable to introduce "correct" employment objects in the data source, but we
still wish to rectify some values, an extra CSV file or SQL table can be used to override
values for specific employments.

If a CSV file is used, it could look something like this:

```
teacher,schoolUnit,employmentRole
baje,12345678,Rektor
anan,11111111,Lärare
```

The two first columns contain values that can be used to uniquely identify a user and a
school unit and the third column contains an attribute which we want to create in the generated
Employment objects.

Whenever we create an Employment object, we will create this attribute (employmentRole in the
example above) with a value from the CSV file if a matching row can be found for the generated
Employment object (i.e. matching the teacher and school unit for the Employment object). For
Employment objects that don't have a matching line in the file, we can first specify a
fall-back attribute to use from the Teacher object, and if that doesn't exist we can also
specify a static value to use for all other Employment objects.

The configuration file for Employment may then have some extra configuration variables,
so that the client can understand the CSV file:

```
# Where to find the extra values
Employment-extra-csv = /workdir/data/employments.csv

# Which column identifies the user?
Employment-extra-Teacher-column = teacher

# Which attribute for the Teacher objects should be used to find teachers?
Employment-extra-Teacher-attribute = uid

# Which column identifies the school unit
Employment-extra-SchoolUnit-column = schoolUnit

# Which attribute for the SchoolUnit objects should be used to find the school units?
Employment-extra-SchoolUnit-attribute = schoolUnitCode

# Attribute to fall back on when there is no match in the extra data
Employment-extra-employmentRole-default = Teacher.employeeType

# Static value to fall back on when there's no extra data and no default attribute
Employment-extra-employmentRole-static = Lärare
```

If you wish to read this data from SQL instead, make sure to configure the SQL connection
(as when loading other data from SQL) and then specify an SQL query instead of a path to a
CSV file:

```
Employment-extra-sql = SELECT * FROM Employment
```

## Limiting the load process

Which objects to load from the source is typically specified with an LDAP filter
or SQL query as described above.
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
gives the path to that file. The third variable specifies which attribute to match against
the values in the list.

Only those objects which match one of the values in the file will be included in the load process.

If you want to match against UUID the `limit-by` variable should be omitted.

### Regular expressions

You can also limit the objects to load to those matching a regular expression. For instance,
if you'd like to only load StudentGroups where the cn attribute includes the text "-klass-":

```
StudentGroup-limit-with = regex
StudentGroup-limit-regex = .*-klass-.*
StudentGroup-limit-by = cn
```

Regular expressions follow the grammar defined in ECMA-262 (as used in JavaScript).

### Combining load limiters

It's possible to combine several load limiters for a type. For instance to limit
groups by matching their names against regular expressions and also by matching
their owning school units against a list.

To use combined load limiters a JSON format is used, the regular expression
example from above would be configured with:

```
StudentGroup-limit = <?
{
  "with": "regex",
  "regex": ".*-klass-.*",
  "by": "cn"
}
?>
```

Limiters can be combined with the logical operators AND, OR and NOT.

For example to load all groups NOT included in a list:

```
StudentGroup-limit = <?
{
  "with": "not",
  "child": {
    "with": "list",
    "list": "groups.txt"
  }
}
?>
```

To load all groups where the group name matches a regular expression
AND their owning school unit is included in a list:

```
StudentGroup-limit = <?
{
  "with": "and",
  "children": [
   {
     "with": "list",
     "list": "schoolunits.txt",
     "by": "owner"
   },
   {
     "with": "regex",
     "regex": ".*-klass-8?$",
     "by": "groupName"
   }
  ]
}
?>
```

OR works like above except "and" is replaced by "or".

### Limiting by endpoint

In the load limiting examples above the limiters have been specified by
EGIL type (e.g. Teacher or StudentGroup). Sometimes it's preferable
to do the limiting by SS12000 type instead. You can then use the name
of the SCIM endpoint instead of EGIL type. So to limit all users with
the same limiter you could use:

```
Users-limit-with = list
Users-limit-list = users.txt
```

This limiter will then be used for any type that has `Users` as their
SCIM endpoint (unless a limiter is specified for the EGIL type, in which
case that is used instead).

### Filtering out orphans

If you wish to remove objects that were loaded but didn't have necessary relations to other
objects you can use the `orphan-if-missing` variable for that data type in question. For instance,
to filter out Teachers without SchoolUnits you can specify:

```
Teacher-orphan-if-missing = SchoolUnit
```

To filter out StudentGroups without members:

```
StudentGroup-orphan-if-missing = Student Teacher
```

Only groups that are missing _both_ Student and Teacher relations will then be filtered out.

## Transforming attributes

Sometimes the attributes in the data source don't have the format we require. Some tranformations
can be carried out in the JSON templates, for instance concatenating strings or with the switch
construct. If the data source is an SQL database the SQL queries can also be used to
transform data. A third method is to specify transform rules which are applied to the data after
it has been read from the data source (but before relations have been established or load limiting
had been done).

Transform rules are specified by data type. Each data type can have multiple rules, they are
carried out in order after an object of that type has been loaded from the data source.

Transform rules are specified as a JSON array, for instance to specify rules for Student
objects:

```
Student-transform-attributes = <?
[
  {
    <<< RULE 1 >>>
  },
  {
    <<< RULE 2 >>>
  }
]
?>
```

Each rule will have a "from" field specifying which attribute to transform. You can also
specify a "function" field to choose which type of transform to perform (if you don't
specify a function the default is to perform a regular expression transform).

### Regular expression transforms

When transforming with regular expressions each rule can have one or many regular expression
to match against. For each regular expression you can also specify how to transform the values
and to which new attribute the transformed value should be written.

If you specify several regular expressions, you can choose whether to apply all transforms or
just the first one to match.

You can also specify that the value should simply be copied to a new attribute if none of the
expressions match.

A simple example for removing a prefix from group names:

```
StudentGroup-transform-attributes = <?
[
  {
    "from": "cn",
    "transforms": [ ["XY-(.*)", "displayName", "$1"] ]
  }
]
?>
```

Here we are transforming the `cn` attribute. For all values matching the regular expression
`XY-(.*)` we will create a new value in the displayName attribute with just the text after
"XY-". Note that regular expression capture groups are used and `$1` in this example refers
to the first capture group (`(.*)`).

If we want to do the same but simply copy values that don't start with "XY-" we can use
the `noMatch` field:

```
StudentGroup-transform-attributes = <?
[
  {
    "from": "cn",
    "transforms": [ ["XY-(.*)", "displayName", "$1"] ],
    "noMatch": "displayName"
  }
]
?>
```

In other words, if none of the expressions under `transforms` match, just copy the value to
`displayName`.

Here's an example with two regular expressions, for splitting a value into two new attributes
(for instance `fullName` into `firstName` and `lastName`):

```
Student-transform-attributes  = <?
[
  {
    "from": "fullName",
    "transforms": [ ["(.*?) .*", "firstName", "$1"],
                    [".*? (.*)", "lastName", "$1"] ]
  }
]
?>
```

You can also set `allMatch` to false if you only want to apply the first matching regular
expression. Here's an example which classifies a group into different types depending on
an expected naming conventions:

```
Student-transform-attributes  = <?
[
  {
    "from": "groups",
    "transforms": [ ["[0-9][a-z]-(.*)", "class", "$1"],
                    ["..-(.*)", "studyGroup", "$1"],
		    ["(.*?)-(.*)", "otherGroup", "$0"] ],
    "allMatch": false
  }
]
?>
```

In this example we need to specify `allMatch` so that for instance classes are _only_
classified as classes even though they also match the more general rules for study groups
and other groups. The expressions are tried in order and only the first match will be
applied.

### URL decode transforms

If the attribute values are URL encoded, the function "urldecode" can be used to
decode the values. For instance:

```
Student-transform-attributes = <?
[
 {
  "from": "extensionAttribute7",
  "to": "classDecoded",
  "function": "urldecode"
 }
]
?>
```

The example above will transform the attribute `extensionAttribute7` by URL decoding
it and write the decoded values to the attribute `classDecoded`. If the `to` attribute
already existed it will be overwritten. If `to` isn't specified the `from` attribute
will be transformed in-place.

## UUIDs

The client will by default warn if an object is discovered to have a bad UUID.

This includes strings that cannot be parsed as a UUID according to RFC4122
and UUIDs that contain upper case letters (according to RFC4122 we should
only send along UUIDs in lower case to the SCIM server).

Since different SCIM servers may handle bad UUIDs differently, and treat
case sensitivity differently, it is highly recommended to only transmit
proper UUIDs in lower case.

The warning can be disabled:

```
disable-bad-uuid-warnings = true
```

This is really only recommended to use if you have an existing integration
which works well with the bad UUIDs and you don't want to fix it.

You can also prevent any objects with bad UUIDs to be loaded:

```
discard-objects-with-bad-uuids = true
```

This is recommended to always use for new configurations so as to not
accidentally send an object with bad UUID to a server.

## Thresholds

Configurable thresholds can be used to protect against sending unintended
large changes. After initially provisioning all intended objects an integration
typically only transfers a few changes every time it runs. By setting
thresholds the EgilSCIMClient will refuse to run if there is suddenly a large
change in the number of objects.

This can protect both against problems in the data source (for instance
all students accidentally disappearing from the data source), and problems
with selections (for instance if your configured rule for selecting groups
and users suddenly by accident selects too many objects).

The easiest way to configure a threshold looks like this:

```
Student-threshold = 50
```

This means that if running the EgilSCIMClient would lead to a change in
objects exceeding 50 Students, the run will be aborted without sending any
changes. Note that this applies both if the number of Students decrease by
50 as well as if it increases by 50.

You can also specify a threshold which applies to all object types:

```
Object-threshold = 100
```

If `Object-threshold` is used together with type specific thresholds, the
type specific thresholds will be used for the types that have them, and
`Object-threshold` will be used for types without type specific thresholds.

If you wish you can also specify relative thresholds:

```
Object-threshold-relative = 10
Student-threshold-relative = 20
```

In the example above students are allowed to change by 20% in each run, objects
of other types may only change by 10%.

You can use both absolute and relative thresholds at once, the EgilSCIMClient
will then abort if either threshold is exceeded.

### Exceptions for thresholds

If you have configured thresholds you can temporarily disable them by
running with the `--skip-thresholds` command line parameter.

Thresholds will also be ignored in the following cases:

 * When there is no cache file to compare against (typically the first run)
 * When using the `--skip-load` parameter (typically used to unprovision everything)

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

## TLS settings

For communication with the SCIM server, the minimum TLS version can be
configured with the setting `min-tls-version`, e.g.:

```
min-tls-version = TLSV1.2
```

Valid values are currently `TLSV1.2` and `TLSV1.3` (older versions can be set
but please note that TLS v1.1 and earlier is considered deprecated).

By not specifying a minimum version, you are relying on the defaults in your
installed version of OpenSSL and libcurl.

For TLS v1.2 and earlier you can specify a list of ciphers to consider when
negotiating TLS connections. This is configured with the setting
`tls-cipher-list`. For information about available ciphers and how to specify
them, see
[libcurl's documentation about ciphers](https://curl.se/docs/ssl-ciphers.html)

The default list of ciphers depends on your version of OpenSSL.
