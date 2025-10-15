#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace arkan::thanatos::infrastructure::config
{

// Central configuration object loaded from thanatos.toml and ENV
struct Config
{
    // ===== app =====
    std::string service_name = "Thanatos";
    std::string version = "0.1.3";
    bool debug = false;

    // ===== logging =====
    std::string log_level = "info";
    bool log_to_file = false;
    std::string log_file = "thanatos.log";
    int log_max_files = 3;
    std::size_t log_max_size_bytes = 10 * 1024 * 1024;

    // ===== protocol =====
    std::size_t proto_max_packet = 65536;

    // ===== query server =====
    std::string query_host = "127.0.0.1";
    std::size_t query_max_buf = 1 * 1024 * 1024;  // 1 MiB
    std::vector<std::uint16_t> query_ports;

    // ===== thanatos (RO) =====
    std::string ro_host = "0.0.0.0";
    std::vector<std::uint16_t> login_ports;
    std::vector<std::uint16_t> char_ports;

    // ===== net client/server tunables =====
    std::size_t net_max_write_queue = 1024;
    bool net_tcp_nodelay = true;
    bool net_tcp_keepalive = false;

    // provenance
    std::string loaded_from = "(defaults)";
};

int ToSpdlogLevel(const std::string& level);
Config LoadConfig(const std::string& toml_path);

}  // namespace arkan::thanatos::infrastructure::config
