# Group Extension

This document specifies a possible extension to SimpleSCIM to allow
group management, and references in general.

The extension is based on that every SimpleSCIM Configuration File is
related to a list of entries of one type. One such entry can contain
attributes relating to other entries of possibly different types. To
allow referencing entries of other types specified in another
SimpleSCIM Configuration File, the references are specified in a
variable in the SimpleSCIM Configuration File in a JSON object as
in the following example:

```
scim-references = <?
[
	{
		"name": "students",
		"location": "/etc/SimpleSCIM/config/students.conf",
		"identifier": "cn",
		"constraint": "students.cn in this.member"
	},
	{
		"name": "teachers",
		"location": "/etc/SimpleSCIM/config/teachers.conf",
		"identifier": "cn",
		"constraint": "this.cn in teachers.groupMembership"
	}
]
?>
```

In the above example, the SimpleSCIM Configuration File is for a
group of `students` and `teachers`. The group is referenced to as
`this` in the constraints. The group entries from the data source
have an attribute `member` with zero or more values representing
student entries that are members of the group. In this example,
teachers are not listed in the `member` attribute, but have their own
attribute `groupMembership` that relate them to the group. This shows
how the group management in the data source can be either from group
to members or from members to group and still work in this context.

The two references `students` and `teachers` can then be used in the
template for the SCIM JSON objects as in the following example:

```
scim-object = <?
	"members": [
	${for $s in students}
	{
		"display": "${$s.fullName}",
		"$ref": "${$s.meta.location}",
		"value": "${$s.meta.id}"
	},
	${end}
	${for $t in teachers}
	{
		"display": "${$t.fullName}",
		"$ref": "${$t.meta.location}",
		"value": "${$t.meta.id}"
	},
	${end}
	]
?>
```

The two references `students` and `teachers` are treated like normal
multi valued attributes of the entries, which are evaluated for every
entry. To not be evaluated to an empty set, the referenced objects
must exist in their specified cache files, which means that they have
been created in a remote system with SimpleSCIM. The attributes
`meta.location` and `meta.id` are added to the cache file after an
execution of SimpleSCIM to keep track of their location and id in the
remote system.

In the cache for the group entries, the referenced objects are stored
as special attributes as `ref.students: cn1, ref.students: cn2` to
keep track of which group members have been properly referenced in
the remote system.
