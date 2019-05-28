#include "catch.hpp"

#include "json_template_parser.hpp"

std::set<std::string> find_variables(const std::string& str) {
    return JSONTemplateParser::find_variables(str.begin(), str.end());
}

typedef std::set<std::string> VariableSet;

TEST_CASE("Empty JSON template") {
    auto vars = find_variables("");
    REQUIRE(vars.empty());
}

TEST_CASE("Basic variable expansion") {
    REQUIRE(find_variables("${foo}") == VariableSet{"foo"});
    REQUIRE(find_variables("${foo} ${bar}") == VariableSet{"foo", "bar"});
    REQUIRE(find_variables("${foo} bar") == VariableSet{"foo"});
    REQUIRE(find_variables("${foo} bar ${baz}") == VariableSet{"foo", "baz"});
    REQUIRE(find_variables("{ foo: ${bar}, baz: { x : ${y}}}") == VariableSet{"bar", "y"});
    REQUIRE(find_variables("${ foo}") == VariableSet{"foo"});
    REQUIRE(find_variables("${foo }") == VariableSet{"foo"});
    REQUIRE(find_variables("${ foo }") == VariableSet{"foo"});
}

TEST_CASE("Switch expansion") {
    REQUIRE(find_variables("${switch foo case \"a\": \"b\" case \"c\": \"d\" default: \"e\"}") == VariableSet{"foo"});

    REQUIRE(find_variables("{\n"\
                           "\t'a' : 'b',\n"\
                           "\t'c' : '${ switch\td.e\n"\
                           "                  case \"a\" : \"b\"\n"\
                           "                  default : \"c\"\n"\
                           "}'") == VariableSet{"d.e"});
}


TEST_CASE("Iterative expansion") {
    REQUIRE(find_variables("${for $e in email}\n"  \
                           "{\n"\
                           " 'value': '${$e}'\n"\
                           " 'displa:y' '${displayName}'\n"\
                           "},\n"\
                           "${end}") == VariableSet{"email", "displayName"});

    REQUIRE(find_variables("${for $a $b in a b.c }\n"\
                           "{\n"\
                           " 'value': '${$a}'\n"\
                           " 'display': '${$b}'\n"\
                           "},\n"\
                           "${end}") == VariableSet{"a", "b.c"});
}

TEST_CASE("Nested interative expansion") {
    REQUIRE(find_variables("${for $a in arr}\n"\
                           "${ruben}\n"\
                           "${ for $b $c in simeon levy.juda }\n"\
                           "${dan}\n"\
                           "${end }\n"\
                           "${ naftali}${gad}\n"\
                           "${ end}\n"\
                           "${switch aser case \"isaskar\" : \"sebulon\"}\n"\
                           "josef : ${benjamin}") ==
            VariableSet{"arr", "ruben", "simeon", "levy.juda", "dan",
                    "naftali", "gad", "aser", "benjamin"});
}
