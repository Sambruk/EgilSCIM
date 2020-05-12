#include "catch.hpp"

#include <vector>
#include <string>

#include "scim_json_parse.hpp"
#include "model/base_object.hpp"

using namespace std;

TEST_CASE("Parse empty JSON template") {
    REQUIRE(scim_json_parse("", base_object("Student")) == "");
}

TEST_CASE("Parse multiple JSON templates") {
    base_object obj("Student");
    obj.add_attribute("a", {"a"} );
    obj.add_attribute("t", { "elev" });
    obj.add_attribute("entryDN", { "dc=foo,o=org,ou=personal,ou=pre" });
    obj.add_attribute("type", { "employee3" });

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
    };

    for (auto test_case : test_cases) {
        auto templ = test_case.first;
        auto wanted = test_case.second;
        REQUIRE(scim_json_parse(templ, obj) == wanted);   
    }
}
