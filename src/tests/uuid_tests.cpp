#include "catch.hpp"
#include "utility/utils.hpp"

TEST_CASE("Parse binary") {
    // UUID for the oid namespace
    unsigned char buf[] = {
        0x6b, 0xa7, 0xb8, 0x12, 0x9d, 0xad, 0x11, 0xd1 ,
        0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8
    };

    REQUIRE(uuid_util::instance().un_parse_uuid(buf) ==
            "6ba7b812-9dad-11d1-80b4-00c04fd430c8");
}

TEST_CASE("Parse MS binary") {
    unsigned char buf[] = {
        0x74, 0xbb, 0xe8, 0x0e, 0x01, 0x57, 0xde, 0x43,
        0xb3, 0x90, 0x57, 0x13, 0x54, 0xc0, 0xc7, 0x3f
    };

    REQUIRE(uuid_util::instance().un_parse_ms_uuid(buf) ==
            "0ee8bb74-5701-43de-b390-571354c0c73f");
}
