#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace arkan::thanatos::infrastructure::config
{

// Central configuration object loaded from thanatos.toml and ENV
struct Config
{
    // ===== app =====
    std::string service_name = "Thanatos";
    std::string version = "0.1.0";
    bool debug = true;

    // ===== net (legacy compatibility) =====
    std::string fakeIP = "127.0.0.1";
    std::uint16_t query_port = 5293;
    std::uint16_t ro_port = 5121;

    // ===== thanatos classic =====
    std::uint16_t login_port = 6900;
    std::uint16_t char_port = 6121;

    // ===== openkore bridge =====
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

    // ===== log =====
    std::string log_level = "info";
    bool log_to_file = false;
    std::string log_file = "logs/thanatos.log";
    int log_max_files = 3;
    std::size_t log_max_size_bytes = 2 * 1024 * 1024;

    // ===== metadata =====
    std::string loaded_from = "(defaults)";
};

// Convert human string level to spdlog numeric level (0..6). Defaults to 'info' (2).
int ToSpdlogLevel(const std::string& level);

// Load configuration from TOML file (if exists) and apply ENV overrides.
Config LoadConfig(const std::string& toml_path);

}  // namespace arkan::thanatos::infrastructure::config
