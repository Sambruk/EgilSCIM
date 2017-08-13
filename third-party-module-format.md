# Third Party Module Format

A third part module is a program written in any programming
language. The module communicates with SimpleSCIM through a
pipe with the following specified format. To enable a third
party module, the variable `data-source` must be set to
`pipe` in the configuration file.

The third part module simply writes data to STDOUT in the
following specified format. All line endings must be LF
(0x0A, '\n').

## Format

The first line written to STDOUT must be the string
`SimpleSCIM module`. This is an identifier to ensure
SimpleSCIM that the incoming data is from a third party
module.

The second line must be the total number of **users** `n`
written in decimal (base 10) in ASCII. The remaining data
written to STDOUT will be `n` **users**.

A **user** consists of a set of **attributes**. The first
line of a **user** is the number of **attributes** `o` of
the **user** written in decimal in ASCII. The remaining
data of the **user** will be `o` **attributes**.

An **attribute** consists of an attribute name and a set of
**values**. The first line of an **attribute** is the
attribute name. The second line is the number of **values**
`p` written in decimal in ASCII. The remaining data of the
**attribute** will be `p` **values**.

A **value** consists of its length and its data. The first
line of a **value** is its length `q` written in decimal in
ASCII. The second line of a **value** is its binary data
consisting of `q` bytes, terminated with a newline.

## Example

If the data source is two users `test1` and `test2` with
the attributes `uid`, `name` and `email`, the following
bash script would be a correct third party module:

```bash
# Module identifier
echo "SimpleSCIM module"

# Number of users
echo "2"

# User 1
echo "3"   # Number of attributes

# User 1 Attribute 1
echo "uid" # Attribute name
echo "1"   # Number of values
echo "3"   # Length of value
echo "id1" # Value

# User 1 Attribute 2
echo "name"
echo "1"
echo "5"
echo "test1"

# User 1 Attribute 3
echo "email"
echo "2"
echo "14"
echo "test1@test.com"    # Value 1
echo "17"
echo "test1@example.com" # Value 2

# User 2
echo "3"

# User 2 Attribute 1
echo "uid"
echo "1"
echo "3"
echo "id2"

# User 2 Attribute 2
echo "name"
echo "1"
echo "5"
echo "test2"

# User 2 Attribute 3
echo "email"
echo "2"
echo "14"
echo "test2@test.com"
echo "17"
echo "test2@example.com"
```

With the above script saved as `module.sh`, and a
configuration file `module.conf` with these lines:

```
[...]
data-source = pipe
user-unique-identifier = uid
[...]
```

the system would be executed with:

```
bash module.sh | SimpleSCIM module.conf
```
