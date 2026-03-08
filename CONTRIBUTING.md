#Rules for contributing to this project
Documentation required for behavior/UI changes

For any PR that adds new functionality, changes behavior, or changes how EgilSCIMClient is configured require documentation updates. 
If docs aren’t updated, request changes and specify what docs are missing.

Documentation must follow doc/STYLE_GUIDE.md

## Code conventions
stderr is only used for errors, not logging. EgilAdmin will consider any output to stderr as a failure worth highlighting.
When EgilSCIMClient encounters problems that aren't considered serious enough to terminate the run we
will sometimes write an error and continue (perhaps skipping problematic objects etc.) and return a successful exit status.
In other words, a sucessful exit status should not be interpreted as a run without any issues, but if we
abort the run prematurely that should be indicated with a non-successful exit status.
