#include "catch.hpp"
#include "config_file_parser.hpp"
#include <map>
#include <string>

using std::map;
using std::string;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

config_parser::insert_fn make_inserter(map<string, string>& m) {
    return [&m](const string& k, const string& v) { m[k] = v; };
}

// Parse 'input' and return all key-value pairs produced.
// Throws config_parse_error on syntax errors.
map<string, string> parse(const string& input) {
    map<string, string> result;
    config_parser parser(std::begin(input), std::end(input), make_inserter(result));
    parser.parse();
    return result;
}

// ---------------------------------------------------------------------------
// Grammar overview
//
//   <config>  ::= ( <ws>* <assign>? <comment>? '\n' )*
//   <ws>      ::= ' ' | '\t'
//   <varid>   ::= [-_a-zA-Z0-9]+
//   <assign>  ::= <varid> <ws>* '=' <ws>* <value>
//   <value>   ::= '<?' [^'?>']* '?>'  <ws>*   -- multi-line form
//               | [^'#' '\n']*                 -- single-line form (trailing ws trimmed)
//   <comment> ::= '#' [^\n]*
// ---------------------------------------------------------------------------

// ===========================================================================
// Empty and blank input
// ===========================================================================

TEST_CASE("Config file parser: empty input produces no entries") {
    REQUIRE(parse("").empty());
}

TEST_CASE("Config file parser: blank lines are ignored") {
    // Lines containing only whitespace are allowed and produce no entries
    string input =
        "   \n"
        "\t\n"
        "\n";
    REQUIRE(parse(input).empty());
}

// ===========================================================================
// Comments
// ===========================================================================

TEST_CASE("Config file parser: comment-only line is ignored") {
    // A line starting with '#' is a comment; it produces no entries
    string input = "# this is a comment\n";
    REQUIRE(parse(input).empty());
}

TEST_CASE("Config file parser: comment after assignment is ignored") {
    // Text following '#' on an assignment line is not part of the value
    string input = "key=value # this is a comment\n";
    REQUIRE(parse(input) == map<string, string>{{"key", "value"}});
}

TEST_CASE("Config file parser: '#' immediately after '=' gives empty value") {
    // '#' acts as a comment delimiter even when there is no value text before it
    string input = "key=# comment\n";
    REQUIRE(parse(input) == map<string, string>{{"key", ""}});
}

TEST_CASE("Config file parser: multiple comment lines between assignments") {
    string input =
        "# first comment\n"
        "a=1\n"
        "# second comment\n"
        "b=2\n";
    map<string, string> expected = {{"a", "1"}, {"b", "2"}};
    REQUIRE(parse(input) == expected);
}

// ===========================================================================
// Variable names  (<varid> ::= [-_a-zA-Z0-9]+)
// ===========================================================================

TEST_CASE("Config file parser: variable name with letters, digits, hyphens and underscores") {
    // All characters valid in a variable name
    string input = "my-var_1=hello\n";
    REQUIRE(parse(input) == map<string, string>{{"my-var_1", "hello"}});
}

TEST_CASE("Config file parser: uppercase letters are valid in variable names") {
    string input = "MyVariable=value\n";
    REQUIRE(parse(input) == map<string, string>{{"MyVariable", "value"}});
}

// ===========================================================================
// Optional whitespace around '='
// ===========================================================================

TEST_CASE("Config file parser: whitespace before '=' is allowed") {
    string input = "key   =value\n";
    REQUIRE(parse(input) == map<string, string>{{"key", "value"}});
}

TEST_CASE("Config file parser: whitespace after '=' is allowed") {
    string input = "key=   value\n";
    REQUIRE(parse(input) == map<string, string>{{"key", "value"}});
}

TEST_CASE("Config file parser: whitespace on both sides of '=' is allowed") {
    string input = "key \t=\t value\n";
    REQUIRE(parse(input) == map<string, string>{{"key", "value"}});
}

TEST_CASE("Config file parser: leading whitespace before variable name is allowed") {
    string input = "   key=value\n";
    REQUIRE(parse(input) == map<string, string>{{"key", "value"}});
}

// ===========================================================================
// Single-line values
// ===========================================================================

TEST_CASE("Config file parser: simple single-line assignment") {
    string input =
        "a=b\n"
        "c=d\n";
    map<string, string> expected = {{"a", "b"}, {"c", "d"}};
    REQUIRE(parse(input) == expected);
}

TEST_CASE("Config file parser: trailing whitespace in single-line value is trimmed") {
    string input = "key=value   \n";
    REQUIRE(parse(input) == map<string, string>{{"key", "value"}});
}

TEST_CASE("Config file parser: value containing spaces is preserved") {
    string input = "key=hello world\n";
    REQUIRE(parse(input) == map<string, string>{{"key", "hello world"}});
}

TEST_CASE("Config file parser: empty single-line value is allowed") {
    string input = "key=\n";
    REQUIRE(parse(input) == map<string, string>{{"key", ""}});
}

TEST_CASE("Config file parser: value containing '=' is allowed") {
    // Only the first '=' is the assignment operator; further '=' belong to the value
    string input = "key=a=b=c\n";
    REQUIRE(parse(input) == map<string, string>{{"key", "a=b=c"}});
}

// ===========================================================================
// Multi-line values  (<value> ::= '<?' ... '?>')
// ===========================================================================

TEST_CASE("Config file parser: multi-line value on a single line") {
    // The '<?...?>' delimiters can appear entirely on one line
    string input = "key=<?hello?>\n";
    REQUIRE(parse(input) == map<string, string>{{"key", "hello"}});
}

TEST_CASE("Config file parser: multi-line value spanning several lines") {
    // Content between '<?' and '?>' may span multiple lines; newlines are preserved
    string input =
        "key=<?\n"
        "line1\n"
        "line2\n"
        "?>\n";
    REQUIRE(parse(input) == map<string, string>{{"key", "\nline1\nline2\n"}});
}

TEST_CASE("Config file parser: multi-line value may contain '#' without starting a comment") {
    // Inside '<?...?>', '#' is literal text, not a comment delimiter
    string input = "key=<? # not a comment ?>\n";
    REQUIRE(parse(input) == map<string, string>{{"key", " # not a comment "}});
}

TEST_CASE("Config file parser: empty multi-line value is allowed") {
    string input = "key=<?  ?>\n";
    REQUIRE(parse(input) == map<string, string>{{"key", "  "}});
}

TEST_CASE("Config file parser: whitespace after '?>' closing delimiter is allowed") {
    // Optional whitespace between '?>' and the end-of-line is ignored
    string input = "key=<?value?>   \n";
    REQUIRE(parse(input) == map<string, string>{{"key", "value"}});
}

// ===========================================================================
// Mixed valid input
// ===========================================================================

TEST_CASE("Config file parser: mix of assignments, blank lines and comments") {
    string input =
        "# Configuration file\n"
        "\n"
        "host=localhost\n"
        "port=8080\n"
        "\n"
        "# Database settings\n"
        "db_name=mydb\n";
    map<string, string> expected = {
        {"host", "localhost"},
        {"port", "8080"},
        {"db_name", "mydb"}
    };
    REQUIRE(parse(input) == expected);
}

// ===========================================================================
// Error: missing newline at end of file
// ===========================================================================

TEST_CASE("Config file parser: missing newline at end of file throws") {
    // Every line, including the last, must be terminated by '\n'.
    // A file that does not end with '\n' is a syntax error.
    string input = "key=value";   // no trailing newline
    REQUIRE_THROWS_AS(parse(input), config_parse_error);
}

TEST_CASE("Config file parser: comment not terminated by newline throws") {
    // A comment must be followed by '\n' before end-of-file
    string input = "# unterminated comment";
    REQUIRE_THROWS_AS(parse(input), config_parse_error);
}

// ===========================================================================
// Error: malformed assignment
// ===========================================================================

TEST_CASE("Config file parser: missing '=' in assignment throws") {
    // A variable name must be followed by '='
    string input = "keyvalue\n";
    REQUIRE_THROWS_AS(parse(input), config_parse_error);
}

TEST_CASE("Config file parser: unexpected character on line throws") {
    // A line that starts with a character that is neither whitespace, a varid
    // character, '#', nor '\n' is a syntax error
    string input = "=badstart\n";
    REQUIRE_THROWS_AS(parse(input), config_parse_error);
}

// ===========================================================================
// Error: unterminated multi-line value
// ===========================================================================

TEST_CASE("Config file parser: multi-line value without closing '?>' throws") {
    // A '<?' delimiter must be matched by a closing '?>' before end-of-file
    string input =
        "key=<?value without closing\n"
        "more text\n";
    REQUIRE_THROWS_AS(parse(input), config_parse_error);
}

// ===========================================================================
// Error: content after closing '?>' on the same line
// ===========================================================================

TEST_CASE("Config file parser: non-whitespace text after '?>' on same line throws") {
    // After the closing '?>' only optional whitespace and a newline are permitted
    string input = "key=<?value?> extra\n";
    REQUIRE_THROWS_AS(parse(input), config_parse_error);
}

// ===========================================================================
// Error location reported in exception
// ===========================================================================

TEST_CASE("Config file parser: exception carries line and column of error") {
    // The config_parse_error exception reports where in the file the error occurred
    string input =
        "good=ok\n"
        "bad";     // line 2, missing '=' and no newline
    try {
        parse(input);
        FAIL("expected config_parse_error");
    } catch (const config_parse_error& e) {
        REQUIRE(e.line() == 1);   // zero-based: second line
    }
}
