# JSON Replacement Rules

In input JSON strings, the following replacement rules apply:

## Simple LDAP variable replacement

`${attr-name}` expands to the first value of the LDAP user attribute
`attr-name`. For example, if an LDAP user has the attribute `uid`
with the value `example`, `{"userName": "${uid}"}` would expand to
`{"userName": "example"}`.

## Conditional LDAP variable replacement

```
${switch attr-name case "attr-val1": "replace-val1"
                   ...
                   default: "default-val"}
```

matches the first value of `attr-name` to a specified `case` value
and expands to the specified replacement value. If no `case` value is
matched, the replacement value from the `default` statement is used.
For example, if an LDAP user has the attribute `type`,

```
{"userType": "${switch type case "StuTypeAll": "Student"
                            case "EmpType1": "Teacher"
                            default: "Unknown"}"}
```

would expand to `{"userType": "Student"}` if the value of `type` is
`StuTypeAll`, to `{"userType": "Teacher"}` if the value of `type` is
`EmpType1` and to `{"userType": "Unknown"}` if the value of `type` is
anything else.


## Iterative LDAP variable replacement

If an LDAP attribute `attr-name` has several values, they can all be
accessed by using an iteration statement `${for $i in attr-name}`
`...` `${end}`. For example, if an LDAP user has the attribute
`email` with the values `email1@example.com` and
`email2@example.com`,

```
{
 "email": [
  ${for $e in email}"${$e}", ${end}
 ]
}
```

would expand to

```
{
 "email": [
   "email1@example.com", "email2@example.com", 
 ]
}
```

Both *Simple* and *Conditional* LDAP variable replacements can be
used inside an *Iterative* LDAP variable replacement. *Iterative*
LDAP variable replacements can also be nested, but ensure that the
iteration variable name is only used in one iteration statement in
that case.

## LDAP variable names

The following names: **switch**, **case**, **default**, **for**,
**in** and **end** are reserved keywords meaning that there can be no
LDAP attributes with those names.

There is also a special attribute `scim-id` that can be used to
access the user's unique identifier in SCIM. This attribute can only
be used in `update` and `delete`, since the user must already be
created to have a unique identifier in SCIM.
