#include "catch.hpp"
#include "audit.hpp"

using namespace audit;

TEST_CASE("Object description") {
    auto unparsable = std::make_shared<rendered_object>("1", "Student", "x");
    REQUIRE(object_description(unparsable) == "1 (unparsable JSON)");

    auto student = std::make_shared<rendered_object>("2", "Student", R"xyz({"userName": "baje"})xyz");
    REQUIRE(object_description(student) == "2 baje");

    auto group = std::make_shared<rendered_object>("3", "StudentGroup", R"xyz({"displayName": "6A", "owner": { "value": "0" }})xyz");
    REQUIRE(object_description(group) == "3 6A owner: 0");

    auto employment = std::make_shared<rendered_object>("4", "Employment", R"xyz({"user": { "value": "2" }, "employedAt": { "value": "0"}})xyz");
    REQUIRE(object_description(employment) == "4 user: 2 employed at: 0");

    auto user = std::make_shared<rendered_object>("5", "User", R"xyz({"userName": "pejo"})xyz");
    REQUIRE(object_description(user) ==  "5 pejo");

    auto unknownWithDisplayName = std::make_shared<rendered_object>("6", "Unknown", R"xyz({"displayName": "foo"})xyz");
    REQUIRE(object_description(unknownWithDisplayName) == "6 foo");

    auto unknownWithoutAnythingHelpful = std::make_shared<rendered_object>("7", "Unknown", R"xyz({"foo": "bar"})xyz");
    REQUIRE(object_description(unknownWithoutAnythingHelpful) == "7");
}

TEST_CASE("Log SCIM audit message") {
    auto previous = std::make_shared<rendered_object>("1", "StudentGroup", R"xyz({"displayName": "A"})xyz");
    auto current = std::make_shared<rendered_object>("1", "StudentGroup", R"xyz({"displayName": "B"})xyz");

    REQUIRE(scim_operation_audit_message(true, SCIM_OTHER_FAILURE, SCIM_CREATE, "StudentGroup", "1", nullptr, current) == "Created StudentGroup 1 B");
    REQUIRE(scim_operation_audit_message(false, SCIM_OTHER_FAILURE, SCIM_CREATE, "StudentGroup", "1", nullptr, current) == "Failed to create StudentGroup 1 B");
    REQUIRE(scim_operation_audit_message(false, SCIM_OTHER_FAILURE, SCIM_CREATE, "StudentGroup", "1", nullptr, nullptr) == "Failed to create StudentGroup 1");
    REQUIRE(scim_operation_audit_message(false, SCIM_CONFLICT_FAILURE, SCIM_CREATE, "StudentGroup", "1", nullptr, current) == "Failed to create (conflict) StudentGroup 1 B");

    REQUIRE(scim_operation_audit_message(true, SCIM_OTHER_FAILURE, SCIM_UPDATE, "StudentGroup", "1", nullptr, current) == "Updated StudentGroup 1 B");
    REQUIRE(scim_operation_audit_message(true, SCIM_OTHER_FAILURE, SCIM_UPDATE, "StudentGroup", "1", previous, current) == "Updated StudentGroup 1 B");
    REQUIRE(scim_operation_audit_message(false, SCIM_OTHER_FAILURE, SCIM_UPDATE, "StudentGroup", "1", nullptr, current) == "Failed to update StudentGroup 1 B");
    REQUIRE(scim_operation_audit_message(false, SCIM_OTHER_FAILURE, SCIM_UPDATE, "StudentGroup", "1", nullptr, nullptr) == "Failed to update StudentGroup 1");
    REQUIRE(scim_operation_audit_message(false, SCIM_NOT_FOUND_FAILURE, SCIM_UPDATE, "StudentGroup", "1", previous, current) == "Failed to update (not found) StudentGroup 1 B");

    REQUIRE(scim_operation_audit_message(true, SCIM_OTHER_FAILURE, SCIM_DELETE, "StudentGroup", "1", previous, nullptr) == "Deleted StudentGroup 1 A");
    REQUIRE(scim_operation_audit_message(true, SCIM_OTHER_FAILURE, SCIM_DELETE, "StudentGroup", "1", nullptr, nullptr) == "Deleted StudentGroup 1");
    REQUIRE(scim_operation_audit_message(false, SCIM_OTHER_FAILURE, SCIM_DELETE, "StudentGroup", "1", previous, nullptr) == "Failed to delete StudentGroup 1 A");
    REQUIRE(scim_operation_audit_message(false, SCIM_OTHER_FAILURE, SCIM_DELETE, "StudentGroup", "1", nullptr, nullptr) == "Failed to delete StudentGroup 1");
    REQUIRE(scim_operation_audit_message(false, SCIM_NOT_FOUND_FAILURE, SCIM_DELETE, "StudentGroup", "1", previous, nullptr) == "Failed to delete (not found) StudentGroup 1 A");
}
