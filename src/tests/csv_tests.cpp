#include "catch.hpp"
#include "csv_file.hpp"
#include <sstream>

namespace {

std::shared_ptr<csv_file> load(const std::string& str) {
    std::istringstream is(str);
    return std::make_shared<csv_file>(is);
}

}

TEST_CASE("Simple") {
    std::string csv =
        "a,b,c\r\n"\
        "0,1,2\r\n";

    auto file = load(csv);

    csv_file::row wanted_header{"a","b","c"};
    csv_file::row wanted_row{"0","1", "2"};
    
    REQUIRE(file->get_header() == wanted_header);
    REQUIRE(file->size() == 1);
    REQUIRE((*file)[0] == wanted_row);

    // same but not newline at end of file
    csv =
        "a,b,c\r\n"\
        "0,1,2";

    file = load(csv);
    REQUIRE(file->get_header() == wanted_header);
    REQUIRE(file->size() == 1);
    REQUIRE((*file)[0] == wanted_row);

    // unix style newlines
    csv =
        "a,b,c\n"\
        "0,1,2\n";

    file = load(csv);
    REQUIRE(file->get_header() == wanted_header);
    REQUIRE(file->size() == 1);
    REQUIRE((*file)[0] == wanted_row);

    // unix style newlines and missing newline at end
    csv =
        "a,b,c\n"\
        "0,1,2";

    file = load(csv);
    REQUIRE(file->get_header() == wanted_header);
    REQUIRE(file->size() == 1);
    REQUIRE((*file)[0] == wanted_row);

    // single column with space
    csv =
        "a\n"\
        "foo bar\n"\
        " baz\n";

    wanted_header = csv_file::row{"a"};
    auto wanted_row1 = csv_file::row{"foo bar"};
    auto wanted_row2 = csv_file::row{" baz"};
    file = load(csv);
    REQUIRE(file->get_header() == wanted_header);
    REQUIRE(file->size() == 2);
    REQUIRE((*file)[0] == wanted_row1);
    REQUIRE((*file)[1] == wanted_row2);
    
    // empty fields
    csv =
        "a,b\n"\
        ",1\n"\
        "1,\n"\
        ",\n";

    wanted_header = csv_file::row{"a", "b"};
    wanted_row1 = csv_file::row{"", "1"};
    wanted_row2 = csv_file::row{"1", ""};
    auto wanted_row3 = csv_file::row{"", ""};
    file = load(csv);
    REQUIRE(file->get_header() == wanted_header);
    REQUIRE(file->size() == 3);
    REQUIRE((*file)[0] == wanted_row1);
    REQUIRE((*file)[1] == wanted_row2);
    REQUIRE((*file)[2] == wanted_row3);    
}

TEST_CASE("Improper (non-escaped)") {
    // Incorrect number of fields
    std::string csv =
        "a,b,c\r\n"\
        "0,1\r\n";

    REQUIRE_THROWS_AS(load(csv), csv_file::format_error);

    // Quote in non-escaped field
    csv = "a,b\n"\
        "1\"1,2\n";
    
    REQUIRE_THROWS_AS(load(csv), csv_file::format_error);

    // Malformed newline
    csv = "a,b\r"\
        "1,2\r";

    REQUIRE_THROWS_AS(load(csv), csv_file::format_error);
}

TEST_CASE("Escaped") {
    std::string csv =
        "a,b,c\n"\
        "\"1,1\",2,3\n"\
        "1,\"2,2\",3\n"\
        "1,2,\"3,3\"\n"\
        "1,\"\"\"\",3\n"\
        "1,\"My name is \"\"Bob\"\", what's yours?\",3\n"\
        "1,\"Foo\nBar\",3\n"\
        "1,\"Å\"\"äö\",3\n";

    auto file = load(csv);

    auto wanted_header = csv_file::row{"a", "b", "c"};
    auto wanted_rows = std::vector<csv_file::row>{
        {"1,1", "2", "3"},
        {"1", "2,2", "3"},
        {"1", "2", "3,3"},
        {"1", "\"", "3"},
        {"1", "My name is \"Bob\", what's yours?", "3"},
        {"1", "Foo\nBar", "3"},
        {"1", "Å\"äö", "3"},
    };

    REQUIRE(file->get_header() == wanted_header);
    REQUIRE(file->size() == wanted_rows.size());

    for (size_t i = 0; i < file->size(); ++i) {
        REQUIRE((*file)[i] == wanted_rows[i]);
    }
}

TEST_CASE("Improper (escaped)") {
    // Quote must be first character in field
    std::string csv =
        "a,b,c\n"\
        " \"1\",2,3";

    REQUIRE_THROWS_AS(load(csv), csv_file::format_error);

    // Incorrect use of a single "
    csv =
        "a,b,c\n"\
        "1,\"\"\",3";
    
    REQUIRE_THROWS_AS(load(csv), csv_file::format_error);

    // Same but as last field in record
    csv =
        "a,b,c\n"\
        "1,2,\"\"\"";

    REQUIRE_THROWS_AS(load(csv), csv_file::format_error);    
}
