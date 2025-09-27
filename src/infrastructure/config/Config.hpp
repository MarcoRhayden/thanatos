#pragma once

#include <cstdint>
#include <string>

namespace arkan::poseidon::infrastructure::config
{

struct Config
{
    // ===== app =====
    std::string service_name = "Arkan-Poseidon";
    std::string version = "0.1.0";
    bool debug = true;

    // ===== networking (legacy / compat) =====
    std::string query_host = "127.0.0.1";
    std::uint16_t query_port = 6900;

    std::string ro_host = "127.0.0.1";
    std::uint16_t ro_port = 5121;

    // ===== poseidon classic =====
    std::uint16_t login_port = 6900;
    std::uint16_t char_port = 6121;

    // ===== openkore bridge =====
    std::string openkore_host = "127.0.0.1";
    std::uint16_t openkore_port = 5293;

    // ===== protocol =====
    std::size_t proto_max_packet = 4 * 1024 * 1024;

    // ===== dummy char =====
    std::string dummy_char_name = "Novice";
    std::string dummy_char_map = "new_zone01";
    int dummy_char_x = 53;
    int dummy_char_y = 111;

    // ===== logging =====
    std::string log_level = "info";
    bool log_to_file = false;
    std::string log_file = "logs/poseidon.log";
    int log_max_files = 3;
    std::size_t log_max_size_bytes = 2 * 1024 * 1024;

    // ===== metadata =====
    std::string loaded_from;
};

Config LoadConfig(const std::string& toml_path = "config/poseidon.toml");
int ToSpdlogLevel(const std::string& level);

}  // namespace arkan::poseidon::infrastructure::config
