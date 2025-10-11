#include "Config.hpp"

#include <toml++/toml.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>

#include "shared/Utils.hpp"

namespace fs = std::filesystem;
namespace apc = arkan::thanatos::infrastructure::config;
namespace shd = arkan::thanatos::shared;

// ============================================================================
// spdlog level helper
// ログレベル（文字列）を spdlog の数値レベルへ変換
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
// Local helpers to safely extract values from toml++ nodes
// toml++ の値取得ヘルパー（存在チェック＋型変換＋デフォルト）
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

// clamp value into 0..65535 just in case
// 念のため 0..65535 に丸める
static std::uint16_t clamp_u16(std::int64_t v)
{
    if (v < 0) return 0;
    if (v > 0xFFFF) return 0xFFFF;
    return static_cast<std::uint16_t>(v);
}

static std::uint16_t get_u16(const toml::table& t, const char* key, std::uint16_t defv)
{
    if (auto node = t[key])
    {
        if (auto u = node.value<uint64_t>()) return clamp_u16(static_cast<std::int64_t>(*u));
        if (auto i = node.value<int64_t>()) return clamp_u16(*i);
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
// コンフィグローダー：TOML を読み込み、最後に環境変数で上書き
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
            // Kept for backward-compatibility / 互換のため残置
            if (auto net = tbl["net"].as_table())
            {
                cfg.query_host = get_str(*net, "query_host", cfg.query_host);
                cfg.query_port = get_u16(*net, "query_port", cfg.query_port);

                // client/network tunables (optional)
                cfg.net_max_write_queue = get_u64(*net, "max_write_queue", cfg.net_max_write_queue);
                cfg.net_tcp_nodelay = get_bool(*net, "tcp_nodelay", cfg.net_tcp_nodelay);
                cfg.net_tcp_keepalive = get_bool(*net, "tcp_keepalive", cfg.net_tcp_keepalive);
            }

            // ===== [thanatos] =====
            if (auto pos = tbl["thanatos"].as_table())
            {
                cfg.ro_host = get_str(*pos, "ro_host", cfg.ro_host);
                cfg.login_port = get_u16(*pos, "login_port", cfg.login_port);
                cfg.char_port = get_u16(*pos, "char_port", cfg.char_port);
            }

            // ===== [protocol] =====
            // Backward compat: accept both "max_packet_size" and "max_packet"
            // 互換性のためキー名を2種サポート
            if (auto proto = tbl["protocol"].as_table())
            {
                // prefer max_packet_size if present / まずは既存キー
                cfg.proto_max_packet = get_u64(*proto, "max_packet_size", cfg.proto_max_packet);
                // if "max_packet" exists, override / 代替キーがあれば上書き
                if (proto->contains("max_packet"))
                {
                    cfg.proto_max_packet = get_u64(*proto, "max_packet", cfg.proto_max_packet);
                }
            }

            // ===== [query] =====
            if (auto q = tbl["query"].as_table())
            {
                cfg.query_max_buf = get_u64(*q, "max_buf", cfg.query_max_buf);
            }

            // ===== [log] =====
            if (auto log = tbl["log"].as_table())
            {
                cfg.log_level = get_str(*log, "level", cfg.log_level);
                cfg.log_to_file = get_bool(*log, "to_file", cfg.log_to_file);
                cfg.log_file = get_str(*log, "file", cfg.log_file);
                if (auto n = (*log)["max_files"].value<int64_t>())
                    cfg.log_max_files = std::max(1, static_cast<int>(*n));
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
        // TOML パース失敗時はデフォルト＋ENV 上書きにフォールバック
        cfg.loaded_from = "(parse error, using defaults)";
    }

    // =========================================================================
    // ENV overrides (optional) — keep names stable to avoid breaking scripts
    // 環境変数による上書き。スクリプト互換のためキー名は安定維持
    // =========================================================================

    // app
    if (auto v = shd::getenv_str("ARKAN_THANATOS_SERVICE_NAME")) cfg.service_name = *v;
    if (auto v = shd::getenv_str("ARKAN_THANATOS_VERSION")) cfg.version = *v;
    if (auto v = shd::getenv_bool("ARKAN_THANATOS_DEBUG")) cfg.debug = *v;

    // net legacy
    if (auto v = shd::getenv_str("ARKAN_THANATOS_QUERY_HOST")) cfg.query_host = *v;  // fixed name
    if (auto v = shd::getenv_u16("ARKAN_THANATOS_QUERY_PORT")) cfg.query_port = *v;

    // Thanatos classic
    if (auto v = shd::getenv_str("ARKAN_THANATOS_RO_HOST"))
        cfg.ro_host = *v;  // FIX: was getenv_u16
    if (auto v = shd::getenv_u16("ARKAN_THANATOS_LOGIN_PORT")) cfg.login_port = *v;
    if (auto v = shd::getenv_u16("ARKAN_THANATOS_CHAR_PORT")) cfg.char_port = *v;

    // protocol
    if (auto v = shd::getenv_size("ARKAN_THANATOS_MAX_PKT")) cfg.proto_max_packet = *v;

    // query
    if (auto v = shd::getenv_size("ARKAN_THANATOS_QUERY_MAX_BUF")) cfg.query_max_buf = *v;

    // net client tunables
    if (auto v = shd::getenv_size("ARKAN_THANATOS_NET_MAX_WRITE_QUEUE"))
        cfg.net_max_write_queue = *v;
    if (auto v = shd::getenv_bool("ARKAN_THANATOS_NET_TCP_NODELAY")) cfg.net_tcp_nodelay = *v;
    if (auto v = shd::getenv_bool("ARKAN_THANATOS_NET_TCP_KEEPALIVE")) cfg.net_tcp_keepalive = *v;

    // log
    if (auto v = shd::getenv_str("ARKAN_THANATOS_LOG_LEVEL")) cfg.log_level = *v;
    if (auto v = shd::getenv_bool("ARKAN_THANATOS_LOG_TO_FILE")) cfg.log_to_file = *v;
    if (auto v = shd::getenv_str("ARKAN_THANATOS_LOG_FILE")) cfg.log_file = *v;
    if (auto v = shd::getenv_int("ARKAN_THANATOS_LOG_MAX_FILES"))
        cfg.log_max_files = std::max(1, *v);
    if (auto v = shd::getenv_size("ARKAN_THANATOS_LOG_MAX_SIZE_BYTES")) cfg.log_max_size_bytes = *v;

    return cfg;
}
