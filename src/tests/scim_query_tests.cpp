#include "catch.hpp"

#include <sstream>
#include <algorithm>
#include <iostream>
#include <boost/property_tree/ptree.hpp>

namespace pt = boost::property_tree;

typedef std::function<int(const std::string& url, char **response_data, long *response_code)> GetFunc;

void simplescim_query_impl(const std::string& url, std::vector<pt::ptree>& resources, GetFunc getter, int start_index = 1);

// A GET mock which simply returns the same body and response code every time.
struct static_getter {
    static_getter(const std::string& result, int response_code)
            : static_result(result),
              static_response_code(response_code) {
    }


    int operator()(const std::string& url, char** response_data, long* response_code) {
        *response_data = const_cast<char*>(static_result.c_str());
        *response_code = static_response_code;
        return 0;
    }

    std::string static_result;
    long static_response_code;
};

namespace {
const char* two_results =
    R"(
   {
     "schemas":["urn:ietf:params:scim:api:messages:2.0:ListResponse"],
     "totalResults":2,
     "Resources":[
       {
         "id":"2819c223-7f76-453a-919d-413861904646",
         "userName":"bjensen"
       },
       {
         "id":"c75ad752-64ae-4823-840d-ffa80929976c",
         "userName":"jsmith"
       }
     ]
   }
    )";

const char* no_results =
    R"(
   {
     "schemas":["urn:ietf:params:scim:api:messages:2.0:ListResponse"],
     "totalResults":0
   }
    )";
}

// A GET mock with pagination.
struct paging_getter {
    paging_getter(int total, int page_size) {
        this->total = total;
        this->page_size = page_size;
    }


    int operator()(const std::string& url, char** response_data, long* response_code) {
        requested_urls.push_back(url);

        generate_response();
        *response_data = const_cast<char*>(response.c_str());

        *response_code = 200;
        return 0;
    }

    void generate_response() {
        auto start_index = (requested_urls.size()-1)*page_size + 1;

        std::ostringstream oss;
        oss << "{\"totalResults\":" + std::to_string(total) + ", \"startIndex\":"+std::to_string(start_index) + ", \"Resources\":[\n";

        int nbr_left = total - (requested_urls.size()-1) * page_size;
        auto count = std::min(nbr_left, page_size);

        for (auto i = 0; i < count; ++i) {
            oss << R"(
                       {
                         "id":"2819c223-7f76-453a-919d-413861904646"
                       })";
            if (i + 1 < count) {
                oss << ",";
            }
            oss << "\n";

        }
        oss << "]}";
        response = oss.str();
    }

    int total;
    int page_size;
    std::string response;
    std::vector<std::string> requested_urls;
};


TEST_CASE("No pagination") {
    std::vector<pt::ptree> resources;
    REQUIRE_THROWS_AS(simplescim_query_impl("", resources, static_getter("", 200)),
                                            std::runtime_error);

    REQUIRE_THROWS_AS(simplescim_query_impl("", resources, static_getter("", 404)),
                                            std::runtime_error);
    REQUIRE_NOTHROW(simplescim_query_impl("", resources, static_getter(two_results, 200)));
    REQUIRE(resources.size() == 2);
    REQUIRE(resources[0].get<std::string>("userName") == "bjensen");
    REQUIRE(resources[1].get<std::string>("userName") == "jsmith");

    resources.clear();
    REQUIRE_NOTHROW(simplescim_query_impl("", resources, static_getter(no_results, 200)));
    REQUIRE(resources.size() == 0);
}

TEST_CASE("Pagination") {
    {
        std::vector<pt::ptree> resources;
        paging_getter pg{11, 20};
        REQUIRE_NOTHROW(simplescim_query_impl("", resources,
                                              [&pg](const std::string& url, char** response_data, long* response_code) -> int { return pg(url, response_data, response_code); }));
        REQUIRE(resources.size() == 11);
        REQUIRE(pg.requested_urls.size() == 1);
    }

    {
        std::vector<pt::ptree> resources;
        paging_getter pg{11, 10};
        REQUIRE_NOTHROW(simplescim_query_impl("", resources,
                                              [&pg](const std::string& url, char** response_data, long* response_code) -> int { return pg(url, response_data, response_code); }));
        REQUIRE(resources.size() == 11);
        REQUIRE(pg.requested_urls.size() == 2);
    }

        {
        std::vector<pt::ptree> resources;
        paging_getter pg{15, 5};
        REQUIRE_NOTHROW(simplescim_query_impl("/Users", resources,
                                              [&pg](const std::string& url, char** response_data, long* response_code) -> int { return pg(url, response_data, response_code); }));
        REQUIRE(resources.size() == 15);
        REQUIRE(pg.requested_urls == std::vector<std::string>{ "/Users", "/Users?startIndex=6", "/Users?startIndex=11" });
    }

}
