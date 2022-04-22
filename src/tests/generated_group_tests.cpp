#include "catch.hpp"
#include "generated_group_load.hpp"

TEST_CASE("Parse student group attributes") {
    const auto spec = R"xyz(
    [
      {
        "from": "groupMembership",
        "match": "(.*)#(.*)",
        "uuid": "$1$2",
        "attributes": [ [ "studentGroupType", "Klass" ], [ "schoolUnitName", "$1" ], [ "displayName", "$2" ] ]
      }
    ])xyz";

    auto attributes = parse_student_group_attributes(spec);
    REQUIRE(attributes.size() == 1);
    REQUIRE(attributes[0].from == "groupMembership");
    REQUIRE(std::regex_match("schoolUnit#groupName", attributes[0].match));
    REQUIRE(attributes[0].uuid == "$1$2");
    REQUIRE(attributes[0].attributes.size() == 3);
    REQUIRE(attributes[0].attributes[0].first == "studentGroupType");
    REQUIRE(attributes[0].attributes[1].second == "$1");
}

TEST_CASE("Parse student group attributes failures") {
    const auto missing_from = R"xyz(
    [
      {
        "match": "(.*)#(.*)",
        "uuid": "$1$2",
        "attributes": [ [ "studentGroupType", "Klass" ], [ "schoolUnitName", "$1" ], [ "displayName", "$2" ] ]
      }
    ])xyz";

    REQUIRE_THROWS_AS(parse_student_group_attributes(missing_from), std::runtime_error);

    const auto incomplete_pair = R"xyz(
    [
      {
        "from": "groupMembership",
        "match": "(.*)#(.*)",
        "uuid": "$1$2",
        "attributes": [ [ "studentGroupType", "Klass" ], [ "schoolUnitName" ], [ "displayName", "$2" ] ]
      }
    ])xyz";

    REQUIRE_THROWS_AS(parse_student_group_attributes(incomplete_pair), std::runtime_error);    
}