#include "catch.hpp"
#include "load_limiter_impl.hpp"

using namespace std;

TEST_CASE("Simple regex limiter") {
    attrib_map attrs = {{"x", {"foo", "bar", "baz"}}};
    auto obj1 = base_object{std::move(attrs)};

    REQUIRE(regex_limiter(".*foo.*", "x").include(&obj1));
    REQUIRE(!regex_limiter(".*zoom.*", "x").include(&obj1));
    REQUIRE(regex_limiter(".*", "x").include(&obj1));
    REQUIRE(!regex_limiter(".*", "y").include(&obj1));
}

TEST_CASE("Not limiter") {
    attrib_map attrs = {{"x", {"foo", "bar", "baz"}}};
    auto obj1 = base_object{std::move(attrs)};

    REQUIRE(!not_limiter(make_shared<regex_limiter>(".*oo$", "x")).include(&obj1));
}

TEST_CASE("And limiter") {
    attrib_map attrs = {{"x", {"foo", "bar", "baz"}}, {"y", {"123"}}};
    auto obj1 = base_object{std::move(attrs)};

    vector<shared_ptr<load_limiter>> limiters = {make_shared<regex_limiter>(".*o.*", "x"), make_shared<regex_limiter>(".a.", "x")};

    REQUIRE(and_limiter(limiters).include(&obj1));

    limiters.push_back(make_shared<regex_limiter>("[0-9]*", "y"));

    REQUIRE(and_limiter(limiters).include(&obj1));

    limiters.push_back(make_shared<regex_limiter>(".*x.*", "y"));

    REQUIRE(!and_limiter(limiters).include(&obj1));
}

TEST_CASE("Or limiter") {
    attrib_map attrs = {{"x", {"foo", "bar", "baz"}}, {"y", {"123"}}};
    auto obj1 = base_object{std::move(attrs)};

    vector<shared_ptr<load_limiter>> limiters = {make_shared<regex_limiter>("[0-9]*", "x"), make_shared<regex_limiter>(".q.", "x")};

    REQUIRE(!or_limiter(limiters).include(&obj1));

    limiters.push_back(make_shared<regex_limiter>("[0-9]*", "y"));

    REQUIRE(or_limiter(limiters).include(&obj1));
}