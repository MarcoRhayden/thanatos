#include "Config.hpp"

#include <toml++/toml.h>

#include <algorithm>
#include <filesystem>
#include <string>

#include "shared/Utils.hpp"

namespace fs = std::filesystem;
namespace apc = arkan::poseidon::infrastructure::config;
namespace shd = arkan::poseidon::shared;

#include <algorithm>

// ============================================================================
// spdlog level helper
// Returns an integer level matching spdlog's levels.
// ============================================================================
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

    return 2;  // info
}

// ----------------------------------------------------------------------------
// Local helpers to safely extract values from toml++ nodes
// ----------------------------------------------------------------------------
static std::size_t get_u64(const toml::table& t, const char* key, std::size_t defv)
{
    if (auto node = t[key])
    {
        if (auto u = node.value<uint64_t>()) return static_cast<std::size_t>(*u);
        if (auto i = node.value<int64_t>()) return static_cast<std::size_t>(*i);
    }

    return defv;
}

static std::uint16_t get_u16(const toml::table& t, const char* key, std::uint16_t defv)
{
    if (auto node = t[key])
    {
        if (auto u = node.value<uint64_t>()) return static_cast<std::uint16_t>(*u);
        if (auto i = node.value<int64_t>()) return static_cast<std::uint16_t>(*i);
    }

    return defv;
}

static bool get_bool(const toml::table& t, const char* key, bool defv)
{
    if (auto node = t[key])
    {
        if (auto b = node.value<bool>()) return *b;
    }

    return defv;
}

static std::string get_str(const toml::table& t, const char* key, const std::string& defv)
{
    if (auto node = t[key])
    {
        if (auto s = node.value<std::string>()) return *s;
    }

    return defv;
}

// ============================================================================
// Config loader
//  - Loads TOML values (if file exists), then applies ENV overrides.
// ============================================================================
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
            if (auto app = tbl["app"].as_table())
            {
                cfg.service_name = get_str(*app, "service_name", cfg.service_name);
                cfg.version = get_str(*app, "version", cfg.version);
                cfg.debug = get_bool(*app, "debug", cfg.debug);
            }

            // ===== [net] (legacy) =====
            // Kept for backward-compatibility, commonly used by your current tree
            if (auto net = tbl["net"].as_table())
            {
                cfg.query_host = get_str(*net, "query_host", cfg.query_host);
                cfg.query_port = get_u16(*net, "query_port", cfg.query_port);
                cfg.ro_host = get_str(*net, "ro_host", cfg.ro_host);
                cfg.ro_port = get_u16(*net, "ro_port", cfg.ro_port);

                // client/network tunables (optional)
                cfg.net_max_write_queue = get_u64(*net, "max_write_queue", cfg.net_max_write_queue);
                cfg.net_tcp_nodelay = get_bool(*net, "tcp_nodelay", cfg.net_tcp_nodelay);
                cfg.net_tcp_keepalive = get_bool(*net, "tcp_keepalive", cfg.net_tcp_keepalive);
            }

            // ===== [poseidon] =====
            if (auto pos = tbl["poseidon"].as_table())
            {
                cfg.login_port = get_u16(*pos, "login_port", cfg.login_port);
                cfg.char_port = get_u16(*pos, "char_port", cfg.char_port);
                cfg.ro_port = get_u16(*pos, "ro_port", cfg.ro_port);  // may override legacy
            }

            // ===== [openkore] =====
            if (auto ok = tbl["openkore"].as_table())
            {
                cfg.openkore_host = get_str(*ok, "host", cfg.openkore_host);
                cfg.openkore_port = get_u16(*ok, "port", cfg.openkore_port);
            }

            // ===== [protocol] =====
            // Backward compat: accept both "max_packet_size" and "max_packet"
            if (auto proto = tbl["protocol"].as_table())
            {
                // prefer max_packet_size (existing)
                cfg.proto_max_packet = get_u64(*proto, "max_packet_size", cfg.proto_max_packet);
                // allow alternative key if present
                cfg.proto_max_packet = get_u64(*proto, "max_packet", cfg.proto_max_packet);
            }

            // ===== [query] =====
            // input buffer cap for Query framing
            if (auto q = tbl["query"].as_table())
            {
                cfg.query_max_buf = get_u64(*q, "max_buf", cfg.query_max_buf);
            }

            // ===== [dummy_char] =====
            if (auto dc = tbl["dummy_char"].as_table())
            {
                cfg.dummy_char_name = get_str(*dc, "name", cfg.dummy_char_name);
                cfg.dummy_char_map = get_str(*dc, "map", cfg.dummy_char_map);
                // x/y may be int64, cast to int
                if (auto x = (*dc)["x"].value<int64_t>()) cfg.dummy_char_x = static_cast<int>(*x);
                if (auto y = (*dc)["y"].value<int64_t>()) cfg.dummy_char_y = static_cast<int>(*y);
            }

            // ===== [log] =====
            if (auto log = tbl["log"].as_table())
            {
                cfg.log_level = get_str(*log, "level", cfg.log_level);
                cfg.log_to_file = get_bool(*log, "to_file", cfg.log_to_file);
                cfg.log_file = get_str(*log, "file", cfg.log_file);
                // ints are fine here
                if (auto n = (*log)["max_files"].value<int64_t>())
                    cfg.log_max_files = static_cast<int>(*n);
                if (auto b = (*log)["max_size_bytes"].value<int64_t>())
                    cfg.log_max_size_bytes = static_cast<std::size_t>(*b);
            }
        }
        else
        {
            cfg.loaded_from = "(defaults)";
        }
    }
    catch (...)
    {
        // Any parsing error falls back to defaults + ENV overrides
        cfg.loaded_from = "(parse error, using defaults)";
    }

    // =========================================================================
    // ENV overrides (optional) — keep names stable to avoid breaking scripts
    // =========================================================================

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

    // query
    if (auto v = shd::getenv_size("ARKAN_POSEIDON_QUERY_MAX_BUF")) cfg.query_max_buf = *v;

    // net client tunables
    if (auto v = shd::getenv_size("ARKAN_POSEIDON_NET_MAX_WRITE_QUEUE"))
        cfg.net_max_write_queue = *v;
    if (auto v = shd::getenv_bool("ARKAN_POSEIDON_NET_TCP_NODELAY")) cfg.net_tcp_nodelay = *v;
    if (auto v = shd::getenv_bool("ARKAN_POSEIDON_NET_TCP_KEEPALIVE")) cfg.net_tcp_keepalive = *v;

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
