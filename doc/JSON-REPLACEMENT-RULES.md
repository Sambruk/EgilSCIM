# JSON Replacement Rules

In input JSON strings, the following replacement rules apply:

## Simple variable replacement

`${attr-name}` expands to the first value of the user attribute
`attr-name`. For example, if a user has the attribute `uid` with the
value `example`, `{"userName": "${uid}"}` would expand to
`{"userName": "example"}`.

## Conditional variable replacement

```
${switch attr-name case "attr-val1": "replace-val1"
                   ...
                   default: "default-val"}
```

matches the first value of `attr-name` to a specified `case` value
and expands to the specified replacement value. If no `case` value is
matched, the replacement value from the `default` statement is used.
For example, if a user has the attribute `type`,

```
{"userType": "${switch type case "StuTypeAll": "Student"
                            case "EmpType1": "Teacher"
                            default: "Unknown"}"}
```

would expand to `{"userType": "Student"}` if the value of `type` is
`StuTypeAll`, to `{"userType": "Teacher"}` if the value of `type` is
`EmpType1` and to `{"userType": "Unknown"}` if the value of `type` is
anything else.

## Relational data
Some objects have relations to other objects, such as group has members.
The other objects fields can be referred to with it's type and it's attribute.

```
GroupName.attributeName
```

## Iterative variable replacement

If an attribute `attr-name` has several values, they can all be
accessed by using an iteration statement `${for $i in attr-name}`
`...` `${end}`. For example, if a user has the attribute `email` with
the values `email1@example.com` and `email2@example.com`,

```
{
 "emails": [
  ${for $e in email}
  {
   "value":"${$e}"
  },
  ${end}
 ]
}
```

would expand to

```
{
 "email": [
  
  {
   "value":"email1@example.com"
  },
  
  {
   "value":"email2@example.com"
  }, 
  
 ]
}
```
Often it's required to list more than one field from relational items. E.g. some groups
need id and name for each of it's members. List the relational variables like this:
```
{
 "members": [
  ${for $i $n in Staff.id Staff.name}
  {
   "id":"${$i}",
   "name":"${$n}"
  },
  ${end}
 ]
}
    
```
Might expand to
```
{
 "members": [
  {
   "id":"25E2F4FD-DCB2-40A2-9773-5EA616C9F412",
   "name":"Tor Modem"
  },
  {
   "id":"404AF0A1-0BCE-4A59-9961-53AB7FEFA8DE",
   "name":"Bob The Builder"
  }
 ]
}

```

Both *Simple* and *Conditional* variable replacements can be used
inside an *Iterative* variable replacement. *Iterative* variable
replacements can also be nested, but ensure that the iteration
variable name is only used in one iteration statement in that case.

## Variable names

The following names: **switch**, **case**, **default**, **for**,
**in** and **end** are reserved keywords meaning that there can be no
user attributes with those names.
