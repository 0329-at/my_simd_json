#include "json_parser.hpp"
#include <print>

int main(int argc, char* argv[]) {
    json::parser parser;

    if (argc != 2) {
        std::println("Usage: {} <json_file>", argv[0]);
        return 1;
    }

    auto path = argv[1];
    // Parse JSON data from a file
    try {
        auto file_data = parser.parse_file(path);
        std::println("Parsed JSON from file successfully.");

        // Access and print project details
        auto project = file_data["project"];
        std::println("Project name: {}", project["name"].as<std::string>());
        std::println("Project version: {}", project["version"].as<std::string>());

        // Access and print dependencies
        auto dependencies = project["dependencies"];
        std::println("Dependencies:");
        for (const auto& [key, value] : dependencies.as<json::object>()) {
            std::println(" - {}: version {}", key, value["version"].as<std::string>());
            std::println("   Features:");
            for (const auto& feature : value["features"].as<json::array>()) {
                std::println("     - {}", feature.as<std::string>());
            }
        }

        // Access and print contributors
        auto contributors = project["contributors"];
        std::println("Contributors:");
        for (const auto& contributor : contributors.as<json::array>()) {
            std::println(" - Name: {}", contributor["name"].as<std::string>());
            std::println("   Email: {}", contributor["email"].as<std::string>());
        }

        // Serialize JSON data back to string
        std::string serialized_json = file_data.serialize();
        std::println("Serialized JSON: {}", serialized_json);
    } catch (const std::exception& e) {
        std::println("Error parsing JSON file: {}", e.what());
    }

    return 0;
}