## Releases

### Unreleased
#### Bugfixes
  - Expansion of ${value} in LDAP relations (#23)
  - Better handling of UUIDs (#26)
  - Corrections in how attributes are found in JSON templates (#24)
    (LDAP attributes used e.g. inside of for-loops in JSON templates
    were not found before this fix, and had to be declared hidden)

#### New features
  - Deletes are done in reverse scim-type-send-order order (#4)
  - Tooling for fetching/formatting/verifying metadata (#18)
  - Relative paths in config files (#13)
  - Proper handling of command line arguments (#32)
  - Framework for automated system testing (#12)

