## Releases

## Unreleased
#### Bugfixes
  - Command line arguments can now be in Unicode on Windows (#232)

## v2.21 (2025-12-09)
#### Features
  - Variable expansions in JSON templates can now be escaped (#156)
  - Organisation object can now be generated (no need to read from data source) (#142)
  - User-Agent header is now used in HTTP requests, and the user agent comment can be configured (#242)

#### Bugfixes
  - Improved handling of the removal of trailing commas in JSON (#212)

## v2.20.1 (2025-06-24)
#### Bugfixes
  - If a Teacher's attribute pointing to school units contain duplicates, don't attempt to generate the same Employment twice (#240)

## v2.20 (2025-06-10)
#### Features
  - The load log file can now be configured to include relations (#94)
  - The load log file can now be configured to include skipped objects (#148)
  - Detection of duplicate UUIDs (#185)
  - Configurable HTTP timeouts (#190)

#### Bugfixes
  - Fixed an access violation which could occur when loading from CSV or SQL auxilliary tables (#92)
  - More reliable writing of the status file (#236)

## v2.19 (2025-03-17)
#### Features
  - Contents of the cache file can now be printed in JSON format (#54)
  - New functionality for blacklisting users (#223)
  - Space for the cache file is now pre-allocated before SCIM operations start (#197)

#### Bugfixes
  - Fixed an access violation which could occur when generating activities if no groups had been loaded (#208)
  - When using the --rebuild-cache feature, a failure to GET objects will now terminate the client (#205)

## v2.18 (2024-11-19)
#### Features
  - Configurable thresholds to prevent accidental large changes (#196)
  - Audit logging (#124)

## v2.17.2 (2024-05-24)
#### Features
  - Unit tests build on Debian Bookworm

## v2.17.1 (2024-04-30)
#### Bugfixes
  - In the Windows version a heap corruption problem was fixed in the LDAP code (#211)

## v2.17.0 (2024-02-27)
#### Features
  - Built-in support for converting national test activity names to UUIDs (#206)
  - A new transform function for URL decoding attributes (#200)

#### Bugfixes
  - Transforms can now also be applied for generated (virtual) groups (#202)

## v2.16.0 (2024-01-09)
#### Features
  - Improvement in Activity objects for users with multiple employments (#195)
  - The CSV separator character can now be a tab character (#193)

#### Bugfixes
  - Temporary CA store file for FedTLS now works on Windows (#191)

## v2.15.0 (2023-06-13)
#### New features
  - Functionality for adding/overriding information to Employment objects (#187)

#### Bugfixes
  - Adds a missing use of readable-id (#184)

## v2.14.0 (2023-04-20)
#### New features
  - Configurable attribute to use to refer to an object in log or error
    messages (#162)
  - Load limiting can be done by endpoint instead of EGIL type (#179)

#### Bugfixes
  - Configurable warning for missing generate key attribute for Employment (#182)

## v2.13.0 (2023-03-13)
#### New features
  - Function for unprovisioning all data (#169)
  - Improved handling of bad UUIDs (#163)

#### Bugfixes
  - fetch_metadata.py has more robust handling of a bad cached file (#168)
  - Improved error message when a DELETE fails with 404 (#167)
  - Don't introduce double '/' when building SCIM URLs (#174)

## v2.12.0 (2022-12-08)
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
