#include "catch.hpp"
#include "external_process_load.hpp"

TEST_CASE("Splitter - single object") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = R"([{"name":"Alice"}])";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content == R"({"name":"Alice"})");
}

TEST_CASE("Splitter - multiple objects") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = R"([{"a":"1"},{"b":"2"},{"c":"3"}])";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content == R"({"a":"1"}{"b":"2"}{"c":"3"})");
}

TEST_CASE("Splitter - whitespace between objects") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = "  [  { \"x\" : 1 } , { \"y\" : 2 }  ]  ";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content == "{ \"x\" : 1 }{ \"y\" : 2 }");
}

TEST_CASE("Splitter - braces inside strings") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = R"([{"val":"a{b}c"}])";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content == R"({"val":"a{b}c"})");
}

TEST_CASE("Splitter - escaped quotes in strings") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = R"([{"val":"say \"hello\""}])";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    std::string expected = R"({"val":"say \"hello\""})";
    REQUIRE(inner.content == expected);
}

TEST_CASE("Splitter - nested objects") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = R"([{"a":{"b":"c"}}])";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content == R"({"a":{"b":"c"}})");
}

TEST_CASE("Splitter - empty array") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = "[]";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content.empty());
}

TEST_CASE("Splitter - completely empty") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = "";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content.empty());
}

TEST_CASE("Splitter - only whitespace") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = "   \t\n  ";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content.empty());
}

TEST_CASE("Splitter - single object without array") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = R"({"a":"b"})";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content == R"({"a":"b"})");
}

TEST_CASE("Splitter - chunked input") {
    string_sink inner;
    json_array_splitter splitter(inner);

    // Send data in small chunks
    std::string input = R"([{"name":"Alice"},{"name":"Bob"}])";
    for (size_t i = 0; i < input.size(); ++i) {
        splitter.write(&input[i], 1);
    }
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content == R"({"name":"Alice"}{"name":"Bob"})");
}

TEST_CASE("Splitter - data after closing bracket is ignored") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = R"([{"a":"1"}] extra stuff)";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content == R"({"a":"1"})");
}

TEST_CASE("Splitter - non-object in array fails") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = R"(["just a string"])";
    splitter.write(input.data(), input.size());
    REQUIRE(splitter.failed());
}

TEST_CASE("Splitter - escaped backslash before quote") {
    string_sink inner;
    json_array_splitter splitter(inner);
    // The value ends with a literal backslash: "path\\"
    // In JSON: "path\\\\" means the string value is path\\ .
    // The closing quote is NOT escaped because \\\\ is two escaped backslashes.
    std::string input = R"([{"val":"path\\"}])";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content == R"({"val":"path\\"})");
}

TEST_CASE("Splitter - NDJSON") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = "{\"a\":\"1\"}\n{\"b\":\"2\"}\n{\"c\":\"3\"}\n";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content == R"({"a":"1"}{"b":"2"}{"c":"3"})");
}

TEST_CASE("Splitter - concatenated objects without delimiter") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = R"({"a":"1"}{"b":"2"}{"c":"3"})";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content == R"({"a":"1"}{"b":"2"}{"c":"3"})");
}

TEST_CASE("Splitter - concatenated objects without delimiter, multiline objects") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = "{\n  \"a\": \"1\"\n}\n{\n  \"b\": \"2\"\n}\n";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(inner.content == "{\n  \"a\": \"1\"\n}{\n  \"b\": \"2\"\n}");
}

TEST_CASE("Splitter - bracket outside array fails") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = R"({"a":"1"}])";
    splitter.write(input.data(), input.size());
    REQUIRE(splitter.failed());
}

TEST_CASE("Splitter - comma outside array fails") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = R"({"a":"1"},{"b":"2"})";
    splitter.write(input.data(), input.size());
    REQUIRE(splitter.failed());
}

TEST_CASE("Splitter - truncated object is detected as incomplete") {
    string_sink inner;
    json_array_splitter splitter(inner);
    std::string input = R"([{"a":"1"}, {"b":)";
    splitter.write(input.data(), input.size());
    REQUIRE_FALSE(splitter.failed());
    REQUIRE(splitter.incomplete());
    REQUIRE(inner.content == R"({"a":"1"})");
}
