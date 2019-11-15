## Releases

### Unreleased
#### New features
  - Functionality for rebuilding the cache file (e.g. if lost) (#10)
  - Support for limiting which objects to load with a list of values (#46)
  - Import from CSV files (#49)
  - It's now possible to build on Windows (with Visual C++) (#57)
  - Support for interpreting UUIDs from Active Directory properly (#59)

#### Bugfixes
  - Failed updates (PUT) are handled better (#42)
  - Relations with method = "object" now handles multi-valued attributes (#50)
  - LDAP-parameters are no longer necessary when using method = "object" (#51)
  - Fixed a bug that could lead to segmentation fault when a delete failed (#62)
  - Activity and Employment objects are now included in load log file (#47)

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
