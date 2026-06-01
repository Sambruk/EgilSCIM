#include "catch.hpp"

#include "config_file.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace filesystem = std::filesystem;

namespace {

// Needed since config_file is a singleton and we want to ensure that each test starts with a clean state. 
// The destructor also ensures that the state is cleared after the test.
// If we change config_file to not be a singleton, this guard can be removed and the tests can just create a
// new config_file instance for each test.
class config_file_guard {
public:
    config_file_guard() {
        config_file::instance().clear();
    }

    ~config_file_guard() {
        config_file::instance().clear();
    }
};

// Helper class to create a temporary directory for the tests. 
// The directory is automatically removed when the object goes out of scope.
class temporary_directory {
public:
    temporary_directory() {
        const auto base = filesystem::canonical(filesystem::temp_directory_path());
        const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();

        for (int i = 0; i < 100; ++i) {
            path = base / ("egilscim-config-file-tests-" + std::to_string(stamp) + "-" + std::to_string(i));
            std::error_code ec;
            if (filesystem::create_directories(path, ec)) {
                return;
            }
        }

        throw std::runtime_error("Failed to create temporary directory");
    }

    ~temporary_directory() {
        std::error_code ec;
        filesystem::remove_all(path, ec);
    }

    filesystem::path path;
};

// Helper function to write content to a file. It also creates the parent directories if they don't exist.
void write_file(const filesystem::path& path, const std::string& content) {
    filesystem::create_directories(path.parent_path());
    std::ofstream file(path);
    REQUIRE(file.is_open());
    file << content;
    file.close();
}

std::string to_utf8_string(const filesystem::path& path) {
    return path.u8string();
}

} // namespace

TEST_CASE("Config file path variables are resolved only by get_path and get_paths") {
    config_file_guard guard;
    temporary_directory temp;

    const auto config_dir = temp.path / "config";
    const auto relative_file = config_dir / "relative.txt";
    const auto vector_relative_file = config_dir / "nested" / "relative2.txt";
    const auto absolute_file = temp.path / "absolute" / "absolute.txt";
    const auto config_path = config_dir / "settings.conf";

    write_file(relative_file, "relative file\n");
    write_file(vector_relative_file, "vector relative file\n");
    write_file(absolute_file, "absolute file\n");

    write_file(config_path,
               "plain-relative=relative.txt\n"
               "plain-absolute=" + to_utf8_string(absolute_file) + "\n"
               "path-list=relative.txt, " + to_utf8_string(absolute_file) + ", nested/relative2.txt\n");

    REQUIRE(config_file::instance().load(to_utf8_string(config_path)) == 0);

    REQUIRE(config_file::instance().get("plain-relative") == "relative.txt");
    REQUIRE(config_file::instance().get("plain-absolute") == to_utf8_string(absolute_file));

    REQUIRE(config_file::instance().get_path("plain-relative") == to_utf8_string(filesystem::absolute(config_dir / "relative.txt")));
    REQUIRE(config_file::instance().get_path("plain-absolute") == to_utf8_string(absolute_file));

    const auto paths = config_file::instance().get_paths("path-list");
    REQUIRE(paths.size() == 3);
    REQUIRE(paths[0] == to_utf8_string(filesystem::absolute(config_dir / "relative.txt")));
    REQUIRE(paths[1] == to_utf8_string(absolute_file));
    REQUIRE(paths[2] == to_utf8_string(filesystem::absolute(config_dir / "nested/relative2.txt")));
}

TEST_CASE("Config file loads template from a relative path") {
    config_file_guard guard;
    temporary_directory temp;

    const auto config_dir = temp.path / "config";
    const auto template_path = config_dir / "templates" / "teacher-template.conf";
    const auto config_path = config_dir / "settings.conf";

    write_file(template_path,
               "Teacher-scim-json-template={\"externalId\":\"${externalId}\"}\n");
    write_file(config_path,
               "Teacher-scim-conf=templates/teacher-template.conf\n");

    REQUIRE(config_file::instance().load(to_utf8_string(config_path)) == 0);
    REQUIRE(config_file::instance().get("Teacher-scim-json-template") == "{\"externalId\":\"${externalId}\"}");
    REQUIRE(config_file::instance().get("Teacher-scim-variables") == "externalId");
    REQUIRE(config_file::instance().get("all-scim-variables") == "externalId");
}

TEST_CASE("Config file loads template from an absolute path") {
    config_file_guard guard;
    temporary_directory temp;

    const auto config_dir = temp.path / "config";
    const auto template_path = temp.path / "templates" / "student-template.conf";
    const auto config_path = config_dir / "settings.conf";

    write_file(template_path,
               "Student-scim-json-template={\"id\":\"${studentId}\"}\n");
    write_file(config_path,
               "Student-scim-conf=" + to_utf8_string(template_path) + "\n");

    REQUIRE(config_file::instance().load(to_utf8_string(config_path)) == 0);
    REQUIRE(config_file::instance().get("Student-scim-json-template") == "{\"id\":\"${studentId}\"}");
    REQUIRE(config_file::instance().get("Student-scim-variables") == "studentId");
    REQUIRE(config_file::instance().get("all-scim-variables") == "studentId");
}
