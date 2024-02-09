#include "catch.hpp"

#include "dnpactivities.hpp"
#include "model/base_object.hpp"
#include "generated_load.hpp"

const std::string definition = R"(
[
  {
    "id": "b229977a-7bd3-58ad-b7a2-3fdc774840fd",
    "displayName": "GRGRMAT01_6",
    "activityType": "Provaktivitet",
    "schoolType": "GR",
    "schoolYear": 6
  },
  {
    "id": "25fe03a1-404d-505e-95ad-329ba05d2af6",
    "displayName": "GRGRMAT01_9",
    "activityType": "Provaktivitet",
    "schoolType": "GR",
    "schoolYear": 9
  }
]
)";

TEST_CASE("dnpactivities") {
    DNPActivities activities(definition);

    auto id1 = activities.name_to_id("GRGRMAT01_6");
    if (id1) {
      REQUIRE(id1.get() == std::string("b229977a-7bd3-58ad-b7a2-3fdc774840fd"));
    }
    else {
      REQUIRE(false);
    }

    auto id2 = activities.name_to_id("GRGRMAT01_9");
    if (id2) {
      REQUIRE(id2.get() == std::string("25fe03a1-404d-505e-95ad-329ba05d2af6"));
    }
    else {
      REQUIRE(false);
    }

    auto id3 = activities.name_to_id("foo");
    if (id3) {
      REQUIRE(false);
    }
}

TEST_CASE("dnpsuffixes") {
    attrib_map attrs1 = {
        {"testName", {"GRGRMAT01"}},
        {"schoolType", {"GR"}},
        {"schoolYear", {"6"}}};
    auto group1 = base_object{std::move(attrs1)};

    attrib_map attrs2 = {
        {"schoolYear", {"6"}}};
    attrib_map attrs3 = {
        {"schoolYear", {"7"}}};
    attrib_map attrs4 = {
        {"schoolYear", {"7"}}};

    auto student1 = base_object{std::move(attrs2)};
    auto student2 = base_object{std::move(attrs3)};
    auto student3 = base_object{std::move(attrs4)};
    object_vector students;
    students.push_back(std::make_shared<base_object>(student1));
    students.push_back(std::make_shared<base_object>(student2));
    students.push_back(std::make_shared<base_object>(student3));

    attrib_map attrs5 = {
        {"testName", {"SVASVA03"}},
        {"schoolType", {"VUX"}},
        {"schoolTypePart", {"VUXGY"}},
    };
    auto group2 = base_object{std::move(attrs5)};

    REQUIRE(deduce_suffix(std::make_shared<base_object>(group1), "schoolType", "schoolYear", nullptr) == "_6");
    REQUIRE(deduce_suffix(std::make_shared<base_object>(group1), "schoolType", "schoolYear", std::make_shared<object_vector>(students)) == "_7");
    REQUIRE(deduce_suffix(std::make_shared<base_object>(group2), "schoolType", "schoolYear", nullptr) == "_VUXGY");
    REQUIRE(deduce_suffix(std::make_shared<base_object>(group2), "schoolTypePart", "schoolYear", nullptr) == "_VUXGY");
}
