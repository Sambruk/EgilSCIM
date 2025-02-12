#include "catch.hpp"
#include "rendered_cache_file.hpp"

TEST_CASE("Estimate file size") {
    auto a = std::make_shared<rendered_object>("1", "A", "{}"); // only in cache file (size 28)
    auto b_old = std::make_shared<rendered_object>("2", "B", "{ \"name\": \"foo\"}");
    auto b_new = std::make_shared<rendered_object>("2", "B", "{ \"name\": \"foobar\"}"); // newer is bigger (size 45)
    auto c_old = std::make_shared<rendered_object>("3", "B", "{ \"name\": \"gurka\"}");
    auto c_new = std::make_shared<rendered_object>("3", "B", "{ \"name\": \"\"}"); // newer is smaller
    auto d = std::make_shared<rendered_object>("4", "C", "{ \"size\": 7 }"); // only in current (size 39)

    rendered_object_list current;
    current.add_object(b_new);
    current.add_object(c_new);
    current.add_object(d);

    rendered_object_list cached;
    cached.add_object(a);
    cached.add_object(b_old);
    cached.add_object(c_old);

    auto estimate = rendered_cache_file::size_estimate(current, cached);
    // totalsize = 9 (header) + 8 (number of objects) + 28 (a) + 45 (b_new) + 44 (c_old) + 39 (d) = 173

    REQUIRE(estimate == 173);
}