#include "catch.hpp"

#include "transformer_impl.hpp"

TEST_CASE("regex_transformer") {
    // Simple case, remove prefix from group names (from cn to displayName)
    regex_transform_rule strip_prefix("DL-(.*)", "displayName", "$1");
    regex_transformer strip_cn_prefix("cn", {strip_prefix}, true, "");

    base_object group1("StudentGroup");
    std::vector<std::string> group1_cn({"DL-group1"});
    group1.add_attribute("cn", group1_cn);

    strip_cn_prefix.apply(&group1);

    REQUIRE(group1.get_values("cn") == group1_cn);
    std::vector<std::string> group1_displayName({"group1"});
    REQUIRE(group1.get_values("displayName") == group1_displayName);

    // Test that values that don't match the regex are ignored
    base_object group2("StudentGroup");
    std::vector<std::string> group2_cn({"group2"});
    group2.add_attribute("cn", group2_cn);

    strip_cn_prefix.apply(&group2);
    REQUIRE(!group2.has_attribute_or_relation("displayName"));

    // Now use noMatch to test that values that don't match the regex are copied
    regex_transformer strip_cn_prefix_nomatch("cn", {strip_prefix}, true, "displayName");

    strip_cn_prefix_nomatch.apply(&group2);
    std::vector<std::string> group2_displayName({"group2"});
    REQUIRE(group2.get_values("displayName") == group2_displayName);

    // Simple case for an attribute with multiple values
    regex_transform_rule strip_suffix("(.*)#Vårdnadshavare", "custodianCivicNo", "$1");
    regex_transformer strip_cn_suffix("custodian", {strip_suffix}, true, "");

    base_object student1("User");
    std::vector<std::string> student1_custodian({"1234#Vårdnadshavare", "1111#Vårdnadshavare", "2222", "3333"});    
    student1.add_attribute("custodian", student1_custodian);
    base_object student1_unmodified = student1;

    strip_cn_suffix.apply(&student1);
    std::vector<std::string> student1_custodianCivicNo({"1234", "1111"});
    REQUIRE(student1.get_values("custodianCivicNo") == student1_custodianCivicNo);

    // Now the same but with noMatch
    student1 = student1_unmodified;
    regex_transformer strip_cn_suffix_nomatch("custodian", {strip_suffix}, true, "custodianCivicNo");
    strip_cn_suffix_nomatch.apply(&student1);
    std::vector<std::string> student1_custodianCivicNo_all({"1234", "1111", "2222", "3333"});
    REQUIRE(student1.get_values("custodianCivicNo") == student1_custodianCivicNo_all);

    // Split a single value into different attributes
    // (full name into first name and last name)
    base_object student2("User");
    std::vector<std::string> student2_name({"Anders Andersson"});
    student2.add_attribute("fullName", student2_name);

    regex_transform_rule firstName("(.*?) .*", "firstName", "$1");
    regex_transform_rule lastName(".*? (.*)", "lastName", "$1");
    regex_transformer split_name("fullName", {firstName, lastName}, true, "");

    split_name.apply(&student2);

    std::vector<std::string> student2_firstName({"Anders"});
    REQUIRE(student2.get_values("firstName") == student2_firstName);
    std::vector<std::string> student2_lastName({"Andersson"});
    REQUIRE(student2.get_values("lastName") == student2_lastName);

    // classify group types with allMatch = false so only the first match applies
    // Note that with the rules below, any class would also be considered a studyGroup
    // or an otherGroup, but we only want the first rule to apply.
    regex_transform_rule className("[0-9][a-z]-(.*)", "class", "$1");
    regex_transform_rule studyGroup("..-(.*)", "studyGroup", "$1");
    regex_transform_rule otherGroup("(.*?)-(.*)", "otherGroup", "$0");
    regex_transformer classifyGroups("groups", {className, studyGroup, otherGroup}, false, "");

    std::vector<std::string> student3_groups({"7b-ugglan", "xy-matte3", "ab-engelska2", "fritis-bandy", "malformed"});
    base_object student3("User");
    student3.add_attribute("groups", student3_groups);

    classifyGroups.apply(&student3);

    std::vector<std::string> student3_className({"ugglan"});
    std::vector<std::string> student3_studyGroup({"matte3", "engelska2"});
    std::vector<std::string> student3_otherGroup({"fritis-bandy"});
    REQUIRE(student3.get_values("class") == student3_className);
    REQUIRE(student3.get_values("studyGroup") == student3_studyGroup);
    REQUIRE(student3.get_values("otherGroup") == student3_otherGroup);
}

TEST_CASE("urldecode_transformer") {
    regex_transform_rule strip_prefix("DL-(.*)", "displayName", "$1");
    regex_transformer strip_cn_prefix("cn", {strip_prefix}, true, "");
    urldecode_transformer urldecoder("encoded", "decoded");

    base_object user1("Student");
    std::vector<std::string> values({"https://kommunen.se/12345678/MA%3A7", "MA%3a7", "foo%00bar"});
    user1.add_attribute("encoded", values);

    urldecoder.apply(&user1);

    auto decoded = user1.get_values("decoded");
    REQUIRE(decoded.size() == 3);
    REQUIRE(decoded[0] == "https://kommunen.se/12345678/MA:7");
    REQUIRE(decoded[1] == "MA:7");
    REQUIRE(decoded[2] == "foo");
}