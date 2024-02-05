#include "catch.hpp"

#include "dnpactivities.hpp"
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