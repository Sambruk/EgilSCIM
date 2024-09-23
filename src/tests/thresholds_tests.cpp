#include "catch.hpp"
#include "thresholds.hpp"

TEST_CASE("Absolute") {
    std::optional<int> none;

    REQUIRE_NOTHROW(verify_thresholds_for_type(100, 90, 10, none));
    REQUIRE_THROWS_AS(verify_thresholds_for_type(100, 89, 10, none), threshold_error);
    REQUIRE_NOTHROW(verify_thresholds_for_type(100, 90, 10, 50));
    REQUIRE_THROWS_AS(verify_thresholds_for_type(100, 89, 10, 50), threshold_error);
    REQUIRE_NOTHROW(verify_thresholds_for_type(100, 110, 10, none));
    REQUIRE_THROWS_AS(verify_thresholds_for_type(100, 111, 10, none), threshold_error);
    REQUIRE_NOTHROW(verify_thresholds_for_type(0, 10, 10, none));
    REQUIRE_THROWS_AS(verify_thresholds_for_type(0, 11, 10, none), threshold_error);
    REQUIRE_NOTHROW(verify_thresholds_for_type(50, 50, 0, none));
    REQUIRE_THROWS_AS(verify_thresholds_for_type(50, 49, 0, none), threshold_error);
}

TEST_CASE("Relative") {
    std::optional<int> none;

    REQUIRE_NOTHROW(verify_thresholds_for_type(100, 90, none, 10));
    REQUIRE_THROWS_AS(verify_thresholds_for_type(100, 89, none, 10), threshold_error);
    REQUIRE_NOTHROW(verify_thresholds_for_type(100, 90, 50, 10));
    REQUIRE_THROWS_AS(verify_thresholds_for_type(100, 89, 50, 10), threshold_error);
    REQUIRE_NOTHROW(verify_thresholds_for_type(100, 110, none, 10));
    REQUIRE_THROWS_AS(verify_thresholds_for_type(100, 111, none, 10), threshold_error);
    REQUIRE_THROWS_AS(verify_thresholds_for_type(0, 100, none, 10), threshold_error);
}

TEST_CASE("Count cache objects") {
    auto empty = std::make_shared<rendered_object_list>();

    REQUIRE(count_objects_of_type(nullptr, "Student") == 0);
    REQUIRE(count_objects_of_type(empty, "Student") == 0);

    auto student1 = std::make_shared<rendered_object>("1", "Student", "{}");
    auto student2 = std::make_shared<rendered_object>("2", "Student", "{}");
    auto group1 = std::make_shared<rendered_object>("3", "StudentGroup", "{}");

    auto list = std::make_shared<rendered_object_list>();
    list->add_object(student1);
    list->add_object(student2);
    list->add_object(group1);

    REQUIRE(count_objects_of_type(list, "Student") == 2);
    REQUIRE(count_objects_of_type(list, "StudentGroup") == 1);
    REQUIRE(count_objects_of_type(list, "Teacher") == 0);
}