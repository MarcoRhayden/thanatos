#include "Config.hpp"

#include <toml++/toml.h>

#include <filesystem>
#include <string>

#include "shared/Utils.hpp"

namespace fs = std::filesystem;
namespace apc = arkan::poseidon::infrastructure::config;
namespace shd = arkan::poseidon::shared;

// ---------------------------------------------------------------------
// spdlog level helper
// ---------------------------------------------------------------------
int apc::ToSpdlogLevel(const std::string& level)
{
    const auto l = shd::to_lower(level);
    if (l == "trace") return 0;
    if (l == "debug") return 1;
    if (l == "info") return 2;
    if (l == "warn") return 3;
    if (l == "error") return 4;
    if (l == "critical") return 5;
    if (l == "off") return 6;
    return 2;
}

// ---------------------------------------------------------------------
// Config loader
// ---------------------------------------------------------------------
apc::Config apc::LoadConfig(const std::string& toml_path)
{
    Config cfg;

    try
    {
        if (fs::exists(toml_path))
        {
            auto tbl = toml::parse_file(toml_path);
            cfg.loaded_from = toml_path;

            // ===== [app] =====
            if (auto v = tbl["app"]["service_name"].value<std::string>()) cfg.service_name = *v;
            if (auto v = tbl["app"]["version"].value<std::string>()) cfg.version = *v;
            if (auto v = tbl["app"]["debug"].value<bool>()) cfg.debug = *v;

            // ===== [net] (legacy) =====
            if (auto v = tbl["net"]["query_host"].value<std::string>()) cfg.query_host = *v;
            if (auto v = tbl["net"]["query_port"].value<int64_t>())
                cfg.query_port = static_cast<std::uint16_t>(*v);
            if (auto v = tbl["net"]["ro_host"].value<std::string>()) cfg.ro_host = *v;
            if (auto v = tbl["net"]["ro_port"].value<int64_t>())
                cfg.ro_port = static_cast<std::uint16_t>(*v);

            // ===== [poseidon] =====
            if (auto v = tbl["poseidon"]["login_port"].value<int64_t>())
                cfg.login_port = static_cast<std::uint16_t>(*v);
            if (auto v = tbl["poseidon"]["char_port"].value<int64_t>())
                cfg.char_port = static_cast<std::uint16_t>(*v);
            if (auto v = tbl["poseidon"]["ro_port"].value<int64_t>())
                cfg.ro_port = static_cast<std::uint16_t>(*v);

            // ===== [openkore] =====
            if (auto v = tbl["openkore"]["host"].value<std::string>()) cfg.openkore_host = *v;
            if (auto v = tbl["openkore"]["port"].value<int64_t>())
                cfg.openkore_port = static_cast<std::uint16_t>(*v);

            // ===== [protocol] =====
            if (auto v = tbl["protocol"]["max_packet_size"].value<int64_t>())
                cfg.proto_max_packet = static_cast<std::size_t>(*v);

            // ===== [dummy_char] =====
            if (auto v = tbl["dummy_char"]["name"].value<std::string>()) cfg.dummy_char_name = *v;
            if (auto v = tbl["dummy_char"]["map"].value<std::string>()) cfg.dummy_char_map = *v;
            if (auto v = tbl["dummy_char"]["x"].value<int64_t>())
                cfg.dummy_char_x = static_cast<int>(*v);
            if (auto v = tbl["dummy_char"]["y"].value<int64_t>())
                cfg.dummy_char_y = static_cast<int>(*v);

            // ===== [log] =====
            if (auto v = tbl["log"]["level"].value<std::string>()) cfg.log_level = *v;
            if (auto v = tbl["log"]["to_file"].value<bool>()) cfg.log_to_file = *v;
            if (auto v = tbl["log"]["file"].value<std::string>()) cfg.log_file = *v;
            if (auto v = tbl["log"]["max_files"].value<int64_t>())
                cfg.log_max_files = static_cast<int>(*v);
            if (auto v = tbl["log"]["max_size_bytes"].value<int64_t>())
                cfg.log_max_size_bytes = static_cast<std::size_t>(*v);
        }
        else
        {
            cfg.loaded_from = "(defaults)";
        }
    }
    catch (...)
    {
        cfg.loaded_from = "(parse error, using defaults)";
    }

    // ===== ENV overrides (optional) =====
    // app
    if (auto v = shd::getenv_str("ARKAN_POSEIDON_SERVICE_NAME")) cfg.service_name = *v;
    if (auto v = shd::getenv_str("ARKAN_POSEIDON_VERSION")) cfg.version = *v;
    if (auto v = shd::getenv_bool("ARKAN_POSEIDON_DEBUG")) cfg.debug = *v;

    // net legacy
    if (auto v = shd::getenv_str("ARKAN_POSEIDON_QUERY_HOST")) cfg.query_host = *v;
    if (auto v = shd::getenv_u16("ARKAN_POSEIDON_QUERY_PORT")) cfg.query_port = *v;
    if (auto v = shd::getenv_str("ARKAN_POSEIDON_RO_HOST")) cfg.ro_host = *v;
    if (auto v = shd::getenv_u16("ARKAN_POSEIDON_RO_PORT")) cfg.ro_port = *v;

    // poseidon classic
    if (auto v = shd::getenv_u16("ARKAN_POSEIDON_LOGIN_PORT")) cfg.login_port = *v;
    if (auto v = shd::getenv_u16("ARKAN_POSEIDON_CHAR_PORT")) cfg.char_port = *v;

    // openkore bridge
    if (auto v = shd::getenv_str("ARKAN_POSEIDON_OK_HOST")) cfg.openkore_host = *v;
    if (auto v = shd::getenv_u16("ARKAN_POSEIDON_OK_PORT")) cfg.openkore_port = *v;

    // protocol
    if (auto v = shd::getenv_size("ARKAN_POSEIDON_MAX_PKT")) cfg.proto_max_packet = *v;

    // dummy char
    if (auto v = shd::getenv_str("ARKAN_POSEIDON_DCHAR_NAME")) cfg.dummy_char_name = *v;
    if (auto v = shd::getenv_str("ARKAN_POSEIDON_DCHAR_MAP")) cfg.dummy_char_map = *v;
    if (auto v = shd::getenv_int("ARKAN_POSEIDON_DCHAR_X")) cfg.dummy_char_x = *v;
    if (auto v = shd::getenv_int("ARKAN_POSEIDON_DCHAR_Y")) cfg.dummy_char_y = *v;

    // log
    if (auto v = shd::getenv_str("ARKAN_POSEIDON_LOG_LEVEL")) cfg.log_level = *v;
    if (auto v = shd::getenv_bool("ARKAN_POSEIDON_LOG_TO_FILE")) cfg.log_to_file = *v;
    if (auto v = shd::getenv_str("ARKAN_POSEIDON_LOG_FILE")) cfg.log_file = *v;
    if (auto v = shd::getenv_int("ARKAN_POSEIDON_LOG_MAX_FILES"))
        cfg.log_max_files = std::max(1, *v);
    if (auto v = shd::getenv_size("ARKAN_POSEIDON_LOG_MAX_SIZE_BYTES")) cfg.log_max_size_bytes = *v;

    return cfg;
}
