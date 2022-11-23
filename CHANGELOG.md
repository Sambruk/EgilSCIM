## Releases

## Unreleased
#### New features
  - Functionality for transforming attributes from the data source (#165)

## v2.11.1 (2022-10-05)
#### Bugfixes
  - Orphan filtering applies to all types (#158)

## v2.11.0 (2022-06-02)
#### New features
  - Groups can now be generated from user attributes (#145)
  - Relations can now be marked as required (#146)
  - Optional referential integrity warnings in relations (#147)
#### Bugfixes
  - Better handling of when LDAP says an attribute both exists and has no values (#20)
  - LDAP attribute names can now include the character '-' (#151)

## v2.10.0 (2022-03-25)
#### New features
  - It's now possible to configure whether or not to follow LDAP referrals (#141)

## v2.9.0 (2022-02-11)
#### New features
  - Objects are automatically indexed internally for faster lookup (#139)

## v2.8.1 (2022-01-31)
#### Bugfixes
  - Better warning messages when a teacher's employment points to a
    non-existant school unit (#127)
  - There is no longer a warning if a relation is defined by a
    "remote" attribute that is potentially multi-valued (#131)
  - Rebuild cache will now work even if the send-order includes
    unconfigured types (#115)
  - JSON template rendering errors are now presented better (#129)
  - Improved error message when generating an Activity for a
    StudentGroup which is missing its SchoolUnit relation (#130)
  - Entity IDs are no longer normalized before comparison (#137)

## v2.8.0 (2021-11-23)
#### New features
  - It is now possible to send a static auth key as an HTTP header (#119)

#### Bugfixes
  - Make sure we terminate early if LDAP connection fails (#125)

## v2.7.0 (2021-07-15)
#### New features
  - The cache file now stores objects in JSON format (#114)

#### Bugfixes
  - If a JSON template can't be applied (e.g. syntax error), for an object
    that has previously been sent successfully, we will no longer forget that
    it was previously sent (#116).

## v2.6.0 (2021-05-11)
#### New features
  - Minimum TLS version and cipher suites are now configurable (#97)
  - The warning about missing school units when generating employments
    can now be disabled (#100)
  - As a security best practice, allowed algorithms for JWS verification
    is now limited to only ES256 (which is what Kontosynk uses). (#103)
  - Support for filtering out orphans after the load phase (#108)
  - Load limiting can now be done with regular expressions (#110)
  - Load limiters can now be combined (#112)

#### Bugfixes
  - Load limiting now also applies to auxilliary attributes (#99)

### v2.5.0 (2020-10-05)
#### New features
  - Support for relational databases as data source (through SQL) (#76)

### v2.4.0 (2020-08-13)
#### New features
  - Plugin system for post processing JSON (#86)
  - Optional support for paged LDAP queries (#88)

### v2.3.0 (2020-05-13)
#### New features
  - Support for metadata which uses alg/digest instead of name/value for pins.
    Metadata which uses name/value is still supported.
  - Umask is used (when available) to limit access to the cache file (#79)
  - Support for regular expressions in the switch construct (#78)
  - Log file names can now contain date and time format specifiers (#80)

#### Bugfixes
  - Removed broken support for multiple config files (#65)

### v2.2.0 (2019-12-10)
#### New features
  - Functionality for rebuilding the cache file (e.g. if lost) (#10)
  - Support for limiting which objects to load with a list of values (#46)
  - Import from CSV files (#49)
  - It's now possible to build on Windows (with Visual C++) (#57)
  - Support for interpreting UUIDs from Active Directory properly (#59)
  - New command line arguments for forcing an object to be updated/created (#38)
  - Reduced need to specify hidden attributes (#64)
  - Support for new metadata format with tags (#74)

#### Bugfixes
  - Failed updates (PUT) are handled better (#42)
  - Relations with method = "object" now handles multi-valued attributes (#50)
  - LDAP-parameters are no longer necessary when using method = "object" (#51)
  - Fixed a bug that could lead to segmentation fault when a delete failed (#62)
  - Activity and Employment objects are now included in load log file (#47)
  - The rebuild-cache function now looks at "id" instead of "externalId" (#45)
  - Better handling of when a CREATE fails because the object already existed (#44)

### v2.1.0 (2019-07-02)
#### Bugfixes
  - Expansion of ${value} in LDAP relations (#23)
  - Better handling of UUIDs (#26)
  - Corrections in how attributes are found in JSON templates (#24)
    (LDAP attributes used e.g. inside of for-loops in JSON templates
    were not found before this fix, and had to be declared hidden)
  - Better error handling and error messages (#3, #22)

#### New features
  - Deletes are done in reverse scim-type-send-order order (#4)
  - Tooling for fetching/formatting/verifying metadata (#18)
  - Relative paths in config files (#13)
  - Proper handling of command line arguments (#32)
  - Framework for automated system testing (#12)
  - Option for logging all HTTP traffic to/from the client (#7)
  - Option for logging all LDAP queries (#22)
