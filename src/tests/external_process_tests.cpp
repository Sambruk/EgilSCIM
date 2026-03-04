#include "catch.hpp"
#include "external_process.hpp"
#include "external_process_load.hpp"
#include "config_file.hpp"

TEST_CASE("Build command line - basic") {
    auto cmd = external_process_manager::build_command_line("x_client -f", "--school-units", "");
    REQUIRE(cmd == "x_client -f --school-units");
}

TEST_CASE("Build command line - with secret") {
    auto cmd = external_process_manager::build_command_line("x_client -f", "--school-units", "/run/secrets/token");
    REQUIRE(cmd == "x_client -f --school-units --secret=/run/secrets/token");
}

TEST_CASE("Build command line - no extra args") {
    auto cmd = external_process_manager::build_command_line("x_client -f", "", "");
    REQUIRE(cmd == "x_client -f");
}

TEST_CASE("Build command line - secret only, no extra args") {
    auto cmd = external_process_manager::build_command_line("x_init -b", "", "mysecret");
    REQUIRE(cmd == "x_init -b --secret=mysecret");
}

TEST_CASE("Parse sessions JSON") {
    std::string json = R"([
        {
            "name": "Session 1",
            "command": "x_client -f"
        },
        {
            "name": "Session 2",
            "init": "x_init",
            "secret": "/run/secrets/token",
            "command": "y_client"
        }
    ])";

    string_sink errors;
    external_process_manager manager(errors);
    manager.parse_sessions(json);

    auto& session1 = manager.get_session("Session 1");
    REQUIRE(session1.name == "Session 1");
    REQUIRE(session1.command == "x_client -f");
    REQUIRE(session1.init.empty());
    REQUIRE(session1.cleanup.empty());
    REQUIRE(session1.secret.empty());
    REQUIRE(!session1.temp_dir.empty());
    REQUIRE(std::filesystem::exists(session1.temp_dir));

    auto& session2 = manager.get_session("Session 2");
    REQUIRE(session2.name == "Session 2");
    REQUIRE(session2.command == "y_client");
    REQUIRE(session2.init == "x_init");
    REQUIRE(session2.cleanup.empty());
    REQUIRE(session2.secret == "/run/secrets/token");

    // Cleanup removes temp dirs
    manager.cleanup_sessions();
    REQUIRE(!std::filesystem::exists(session1.temp_dir));
    REQUIRE(!std::filesystem::exists(session2.temp_dir));
    REQUIRE(errors.content.empty());
}

TEST_CASE("Parse sessions - all optional fields are parsed") {
    std::string json = R"([{
        "name": "S",
        "command": "cmd",
        "init": "do_init",
        "cleanup": "do_cleanup",
        "secret": "mysecret"
    }])";
    string_sink errors;
    {
        external_process_manager manager(errors);
        manager.parse_sessions(json);

        auto& s = manager.get_session("S");
        REQUIRE(s.init == "do_init");
        REQUIRE(s.cleanup == "do_cleanup");
        REQUIRE(s.secret == "mysecret");
        // Destructor will attempt cleanup with nonexistent do_cleanup command.
        // Errors go to the string_sink, not stderr.
        // We expect an error because do_cleanup doesn't exist.
    }
    REQUIRE_FALSE(errors.content.empty());
}

// Verify the error sink actually captured the cleanup failure
// after the manager from the previous test was destroyed.
// (Not possible to check across TEST_CASEs in Catch2, so we
// use a scope to force destruction and then check.)
TEST_CASE("Cleanup with nonexistent command reports error") {
    string_sink errors;
    {
        external_process_manager manager(errors);
        manager.parse_sessions(R"([{"name": "S", "command": "cmd", "cleanup": "nonexistent_cleanup"}])");
        // manager destructor runs here, attempting cleanup
    }
    REQUIRE_FALSE(errors.content.empty());
}

TEST_CASE("Run command - session not found") {
    string_sink errors;
    external_process_manager manager(errors);
    manager.parse_sessions(R"([{"name": "S1", "command": "cmd"}])");

    string_sink out, err;
    REQUIRE_THROWS_AS(manager.run_command("nonexistent", "", out, err), std::runtime_error);
    manager.cleanup_sessions();
    REQUIRE(errors.content.empty());
}

TEST_CASE("Parse sessions - invalid JSON") {
    string_sink errors;
    external_process_manager manager(errors);
    REQUIRE_THROWS_AS(manager.parse_sessions("not valid json"), std::runtime_error);
    REQUIRE(errors.content.empty());
}

TEST_CASE("Parse sessions - missing required field") {
    string_sink errors;
    external_process_manager manager(errors);
    // Missing "command" field
    REQUIRE_THROWS(manager.parse_sessions(R"([{"name": "S1"}])"));
    REQUIRE(errors.content.empty());
}

TEST_CASE("JSON to object list - simple objects") {
    config_file::instance().replace_variable("SchoolUnit-unique-identifier", "id");

    std::string json = R"([
        {
            "id": "f80e5b0b-af6b-4797-8726-738a06fffc2c",
            "schoolUnitCode": "12345678",
            "name": "Storskolan"
        },
        {
            "id": "322dfc6d-fba0-4ce8-8c73-cbe3d46f97e9",
            "schoolUnitCode": "11223344",
            "name": "Lillskolan"
        }
    ])";

    auto objects = json_to_object_list(json, "SchoolUnit");
    REQUIRE(objects->size() == 2);

    auto obj1 = objects->get_object("f80e5b0b-af6b-4797-8726-738a06fffc2c");
    REQUIRE(obj1 != nullptr);
    REQUIRE(obj1->get_values("name") == string_vector{"Storskolan"});
    REQUIRE(obj1->get_values("schoolUnitCode") == string_vector{"12345678"});
    REQUIRE(obj1->getSS12000type() == "SchoolUnit");

    auto obj2 = objects->get_object("322dfc6d-fba0-4ce8-8c73-cbe3d46f97e9");
    REQUIRE(obj2 != nullptr);
    REQUIRE(obj2->get_values("name") == string_vector{"Lillskolan"});
}

TEST_CASE("JSON to object list - multi-valued attributes") {
    config_file::instance().replace_variable("StudentGroup-unique-identifier", "id");

    std::string json = R"([
        {
            "id": "f80e5b0b-af6b-4797-8726-738a06fffc2c",
            "name": "Group A",
            "members": ["student1", "student2", "student3"]
        }
    ])";

    auto objects = json_to_object_list(json, "StudentGroup");
    REQUIRE(objects->size() == 1);

    auto obj = objects->get_object("f80e5b0b-af6b-4797-8726-738a06fffc2c");
    REQUIRE(obj != nullptr);
    auto members = obj->get_values("members");
    REQUIRE(members.size() == 3);
    REQUIRE(members[0] == "student1");
    REQUIRE(members[1] == "student2");
    REQUIRE(members[2] == "student3");
}

TEST_CASE("JSON to object list - empty array") {
    auto objects = json_to_object_list("[]", "SchoolUnit");
    REQUIRE(objects->size() == 0);
}

TEST_CASE("JSON to object list - nested object attribute is ignored") {
    config_file::instance().replace_variable("SchoolUnit-unique-identifier", "id");

    std::string json = R"([
        {
            "id": "f80e5b0b-af6b-4797-8726-738a06fffc2c",
            "name": "Storskolan",
            "address": {
                "street": "Storgatan 1",
                "city": "Stockholm"
            }
        }
    ])";

    auto objects = json_to_object_list(json, "SchoolUnit");
    REQUIRE(objects->size() == 1);

    auto obj = objects->get_object("f80e5b0b-af6b-4797-8726-738a06fffc2c");
    REQUIRE(obj != nullptr);
    REQUIRE(obj->get_values("name") == string_vector{"Storskolan"});
    REQUIRE_FALSE(obj->has_attribute_or_relation("address"));
}

TEST_CASE("JSON to object list - invalid JSON") {
    REQUIRE_THROWS_AS(json_to_object_list("not json", "SchoolUnit"), std::runtime_error);
}
