#Rules for contributing to this project
Documentation required for behavior/UI changes

For any PR that adds new functionality, changes behavior, or changes how EgilSCIMClient is configured require documentation updates. 
If docs aren’t updated, request changes and specify what docs are missing.

Documentation must follow doc/STYLE_GUIDE.md

## Code conventions
stderr is only used for errors, not logging. While the EgilSCIMClient should return a correct
exit status, EgilAdmin will consider any output to stderr as a failed run.
