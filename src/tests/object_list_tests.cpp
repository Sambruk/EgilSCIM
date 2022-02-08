#include "catch.hpp"
#include "model/object_list.hpp"

TEST_CASE("Add object") {
    object_list list;
    auto obj = std::make_shared<base_object>("User");
    list.add_object("a", obj);
    REQUIRE(list.size() == 1);
    REQUIRE(list.get_object("a") != nullptr);
    list.remove("a");
    REQUIRE(list.empty());
    REQUIRE(list.get_object("a") == nullptr);
}

TEST_CASE("Replace object") {
    object_list list;
    auto obj1 = std::make_shared<base_object>("User");
    obj1->add_attribute("name", {"foo"});
    auto obj2 = std::make_shared<base_object>("User");
    obj2->add_attribute("name", {"bar"});
    list.add_object("a", obj1);
    REQUIRE(list.size() == 1);
    REQUIRE(list.get_object("a").get() == obj1.get());
    list.add_object("a", obj2);
    REQUIRE(list.size() == 1);
    REQUIRE(list.get_object("a").get() == obj2.get());
}