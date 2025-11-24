#include "catch.hpp"

#include <vector>
#include <string>

#include "scim_json_parse.hpp"
#include "model/base_object.hpp"
#include "config_file.hpp"

using namespace std;

TEST_CASE("Parse empty JSON template") {
    REQUIRE(scim_json_parse("", base_object("Student"), false) == "");
}

TEST_CASE("Parse multiple JSON templates") {
    base_object obj("Student");
    obj.add_attribute("uuid", { "fce5eba1-7c49-46e5-b117-9dd137031680" } );
    obj.add_attribute("a", {"a"} );
    obj.add_attribute("t", { "elev" });
    obj.add_attribute("entryDN", { "dc=foo,o=org,ou=personal,ou=pre" });
    obj.add_attribute("type", { "employee3" });
    obj.add_attribute("displayName", { "Åke \"Ankan\" Åström" });
    obj.add_attribute("tags", { "one", "two", "three" });

    auto test_cases = vector<pair<string,string>> {
        { "",
          "" },
        
        { R"("a": "${a}")",
          R"("a": "a")" },

        { R"("a": "${b}")",
          "" },

        { R"("type": "${switch t case "personal": "Teacher" default: "Student"}")",
          R"("type": "Student")" },

        { R"("type": "${switch entryDN case /.*ou=personal.*/: "Teacher" default: "Student"}")",
          R"("type": "Teacher")" },
 
        { R"("type": "${switch t case /.*ou=personal.*/: "Teacher" default: "Student"}")",
          R"("type": "Student")" },

        { R"("type": "${switch type case /employee[123]/: "Teacher" default: "Student"}")",
          R"("type": "Teacher")" },
        
        { R"("type": "${switch type case /employee[12]/: "Teacher" default: "Student"}")",
          R"("type": "Student")" },

        { R"("displayName": "${|displayName}")",
          R"("displayName": "Åke \"Ankan\" Åström")" },

        { R"("displayName": "${displayName}")",
          R"("displayName": "Åke "Ankan" Åström")" },

        { R"("tags": [${for $t in tags}"${$t}",${end}])",
          R"("tags": ["one","two","three" ])" },
    };

    // TODO: remove this config file stuff once base_object doesn't
    //       depend on config_file.
    config_file &config = config_file::instance();
    config.replace_variable("Student-unique-identifier", "uuid");
    
    for (auto test_case : test_cases) {
        auto templ = test_case.first;
        auto wanted = test_case.second;
        REQUIRE(scim_json_parse(templ, obj, false) == wanted);   
    }
}


std::string json_string_escape(const std::string&);

TEST_CASE("json_string_escape escapes required characters and preserves UTF-8") {
    // Build input containing characters that must be escaped:
    // - double quote
    // - backslash
    // - newline, carriage return, tab, backspace, formfeed
    // - a control character (0x01) that should be encoded as \u0001
    // - UTF-8 non-ASCII characters which must be preserved
    std::string input;
    input += "Quote: ";
    input += '"';
    input += " Backslash: ";
    input += '\\';
    input += " Newline:\n";
    input += " Carriage:\r";
    input += " Tab:\t";
    input += " Backspace:\b";
    input += " Formfeed:\f";
    input += " Ctrl1:";
    input += static_cast<char>(0x01);
    input += " End UTF8: åäö €"; // UTF-8 non-ASCII characters

    std::string escaped = json_string_escape(input);

    // Expected string: control sequences replaced with their escape sequences,
    // control 0x01 becomes \u0001, and UTF-8 text is preserved verbatim.
    std::string expected = R"(Quote: \" Backslash: \\ Newline:\n Carriage:\r Tab:\t Backspace:\b Formfeed:\f Ctrl1:\u0001 End UTF8: åäö €)";

    REQUIRE(escaped == expected);
}
