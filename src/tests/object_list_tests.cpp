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

TEST_CASE("Object index") {
    object_index idx("name");
    auto obj1 = std::make_shared<base_object>(attrib_map{{"name", {"foo"}}});
    idx.add(obj1);

    auto res = idx.lookup("foo");
    REQUIRE(res.size() == 1);
    REQUIRE(res[0].get() == obj1.get());

    res = idx.lookup("bar");
    REQUIRE(res.size() == 0);

    auto obj2 = std::make_shared<base_object>(attrib_map{{"name", {"bar", "foo"}}});
    idx.add(obj2);

    res = idx.lookup("bar");
    REQUIRE(res.size() == 1);
    REQUIRE(res[0].get() == obj2.get());

    res = idx.lookup("foo");
    REQUIRE(res.size() == 2);

    idx.remove(obj1);

    res = idx.lookup("foo");
    REQUIRE(res.size() == 1);
    REQUIRE(res[0].get() == obj2.get());

    res = idx.lookup("bar");
    REQUIRE(res.size() == 1);
    REQUIRE(res[0].get() == obj2.get());

    idx.add(obj1);
    idx.remove(obj2);

    res = idx.lookup("foo");
    REQUIRE(res.size() == 1);
    REQUIRE(res[0].get() == obj1.get());
    
    res = idx.lookup("bar");
    REQUIRE(res.size() == 0);
}

TEST_CASE("Object lookup") {
    object_list list;
    auto obj1 = std::make_shared<base_object>("User");
    obj1->add_attribute("name", {"foo"});
    auto obj2 = std::make_shared<base_object>("User");
    obj2->add_attribute("name", {"bar"});
    list.add_object("a", obj1);
    list.add_object("b", obj2);

    auto res = list.get_object_for_attribute("name", "foo");
    REQUIRE(res != nullptr);
    REQUIRE(res.get() == obj1.get());

    res = list.get_object_for_attribute("name", "bar");
    REQUIRE(res != nullptr);
    REQUIRE(res.get() == obj2.get());

    list.remove("b");

    res = list.get_object_for_attribute("name", "bar");
    REQUIRE(res == nullptr);

    res = list.get_object_for_attribute("name", "foo");
    REQUIRE(res != nullptr);
    REQUIRE(res.get() == obj1.get());

    auto obj3 = std::make_shared<base_object>("User");
    obj3->add_attribute("name", {"baz", "zoink"});
    list.add_object("c", obj3);

    res = list.get_object_for_attribute("name", "zoink");
    REQUIRE(res != nullptr);
    REQUIRE(res.get() == obj3.get());
}