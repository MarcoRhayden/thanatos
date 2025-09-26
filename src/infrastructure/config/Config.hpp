#pragma once
#include <cstdint>
#include <optional>
#include <string>

namespace arkan::poseidon::infrastructure::config
{

struct Config
{
    // app
    std::string service_name = "Arkan-Poseidon";
    std::string version = "0.1.0";
    bool debug = true;

    // networking (placeholders)
    std::string query_host = "127.0.0.1";
    std::uint16_t query_port = 6900;
    std::string ro_host = "127.0.0.1";
    std::uint16_t ro_port = 5121;

    // logging
    std::string log_level = "info";
    bool log_to_file = false;
    std::string log_file = "logs/poseidon.log";
    int log_max_files = 3;
    std::size_t log_max_size_bytes = 2 * 1024 * 1024;

    // info
    std::string loaded_from;
};

Config LoadConfig(const std::string& toml_path = "config/poseidon.toml");
int ToSpdlogLevel(const std::string& level);

}  // namespace arkan::poseidon::infrastructure::config
