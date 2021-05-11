## Releases

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
