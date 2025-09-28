#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace arkan::poseidon::infrastructure::config
{

// Central configuration object loaded from poseidon.toml and ENV
struct Config
{
    // ===== app =====
    std::string service_name = "Arkan-Poseidon";
    std::string version = "0.1.0";
    bool debug = true;

    // ===== net (legacy compatibility) =====
    std::string query_host = "127.0.0.1";
    std::uint16_t query_port = 5293;
    std::string ro_host = "127.0.0.1";
    std::uint16_t ro_port = 5121;

    // ===== poseidon classic =====
    std::uint16_t login_port = 6900;
    std::uint16_t char_port = 6121;

    // ===== openkore bridge =====
    std::string openkore_host = "127.0.0.1";
    std::uint16_t openkore_port = 5293;

    // ===== protocol =====
    // Maximum allowed packet size parsed by domain::protocol::Parser
    std::size_t proto_max_packet = 4 * 1024 * 1024;  // 4 MiB

    // ===== query (NEW) =====
    // Max input buffer for Query framing (to prevent abuse/overflow)
    std::size_t query_max_buf = 1 * 1024 * 1024;  // 1 MiB

    // ===== net client (NEW) =====
    // Backpressure cap and TCP options for AsioTcpClient
    std::size_t net_max_write_queue = 1024;
    bool net_tcp_nodelay = true;
    bool net_tcp_keepalive = true;

    // ===== dummy char =====
    std::string dummy_char_name = "Novice";
    std::string dummy_char_map = "new_zone01";
    int dummy_char_x = 53;
    int dummy_char_y = 111;

    // ===== log =====
    std::string log_level = "info";
    bool log_to_file = false;
    std::string log_file = "logs/poseidon.log";
    int log_max_files = 3;
    std::size_t log_max_size_bytes = 2 * 1024 * 1024;

    // ===== metadata =====
    std::string loaded_from = "(defaults)";
};

// Convert human string level to spdlog numeric level (0..6). Defaults to 'info' (2).
int ToSpdlogLevel(const std::string& level);

// Load configuration from TOML file (if exists) and apply ENV overrides.
Config LoadConfig(const std::string& toml_path);

}  // namespace arkan::poseidon::infrastructure::config
