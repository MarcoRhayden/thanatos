#include "Config.hpp"

#include <toml++/toml.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>

#include "shared/Utils.hpp"

namespace fs = std::filesystem;
namespace apc = arkan::thanatos::infrastructure::config;
namespace shd = arkan::thanatos::shared;

// ============================================================================
// spdlog level helper
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

    return 2;  // default: info
}

// ----------------------------------------------------------------------------
// Helpers
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

static std::uint16_t clamp_u16(std::int64_t v)
{
    if (v < 0) return 0;
    if (v > 0xFFFF) return 0xFFFF;
    return static_cast<std::uint16_t>(v);
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

static void get_ports_array(const toml::table& t, const char* key, std::vector<std::uint16_t>& out)
{
    if (auto node = t[key])
    {
        if (auto arr = node.as_array())
        {
            for (auto&& v : *arr)
            {
                if (auto u = v.value<uint64_t>())
                    out.push_back(clamp_u16(static_cast<std::int64_t>(*u)));
                else if (auto i = v.value<int64_t>())
                    out.push_back(clamp_u16(*i));
            }
        }
    }
}

// ============================================================================
// Config loader (lists-only standard)
// ============================================================================
apc::Config apc::LoadConfig(const std::string& toml_path)
{
    Config cfg;

    if (!fs::exists(toml_path))
    {
        throw std::runtime_error("Config file not found: " + toml_path);
    }

    auto tbl = toml::parse_file(toml_path);
    cfg.loaded_from = toml_path;

    // ===== [app] =====
    if (auto app = tbl["app"].as_table())
    {
        cfg.service_name = get_str(*app, "service_name", cfg.service_name);
        cfg.version = get_str(*app, "version", cfg.version);
        cfg.debug = get_bool(*app, "debug", cfg.debug);
    }

    // ===== [thanatos] =====
    if (auto pos = tbl["thanatos"].as_table())
    {
        cfg.ro_host = get_str(*pos, "ro_host", cfg.ro_host);
        get_ports_array(*pos, "login_ports", cfg.login_ports);
        get_ports_array(*pos, "char_ports", cfg.char_ports);
    }

    // ===== [protocol] =====
    if (auto proto = tbl["protocol"].as_table())
    {
        cfg.proto_max_packet = get_u64(*proto, "max_packet_size", cfg.proto_max_packet);
        if (proto->contains("max_packet"))
            cfg.proto_max_packet = get_u64(*proto, "max_packet", cfg.proto_max_packet);
    }

    // ===== [query] =====
    if (auto q = tbl["query"].as_table())
    {
        cfg.query_host = get_str(*q, "host", cfg.query_host);
        cfg.query_max_buf = get_u64(*q, "max_buf", cfg.query_max_buf);
        get_ports_array(*q, "ports", cfg.query_ports);
    }

    // arrays must be non-empty
    if (cfg.login_ports.empty())
        throw std::runtime_error("thanatos.login_ports is required and must not be empty");
    if (cfg.char_ports.empty())
        throw std::runtime_error("thanatos.char_ports is required and must not be empty");
    if (cfg.query_ports.empty())
        throw std::runtime_error("query.ports is required and must not be empty");

    return cfg;
}
