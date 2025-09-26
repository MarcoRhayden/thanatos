#include "Config.hpp"

#include <toml++/toml.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;
namespace apc = arkan::poseidon::infrastructure::config;

static std::string getenv_str(const char* k)
{
    const char* v = std::getenv(k);
    return v ? std::string(v) : std::string();
}

static std::string to_lower(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

int apc::ToSpdlogLevel(const std::string& level)
{
    auto l = to_lower(level);
    if (l == "trace") return 0;
    if (l == "debug") return 1;
    if (l == "info") return 2;
    if (l == "warn") return 3;
    if (l == "error") return 4;
    if (l == "critical") return 5;
    if (l == "off") return 6;
    return 2;
}

apc::Config apc::LoadConfig(const std::string& toml_path)
{
    Config cfg;
    try
    {
        if (fs::exists(toml_path))
        {
            auto tbl = toml::parse_file(toml_path);
            cfg.loaded_from = toml_path;

            if (auto v = tbl["app"]["service_name"].value<std::string>()) cfg.service_name = *v;
            if (auto v = tbl["app"]["version"].value<std::string>()) cfg.version = *v;
            if (auto v = tbl["app"]["debug"].value<bool>()) cfg.debug = *v;

            if (auto v = tbl["net"]["query_host"].value<std::string>()) cfg.query_host = *v;
            if (auto v = tbl["net"]["query_port"].value<int64_t>())
                cfg.query_port = static_cast<uint16_t>(*v);
            if (auto v = tbl["net"]["ro_host"].value<std::string>()) cfg.ro_host = *v;
            if (auto v = tbl["net"]["ro_port"].value<int64_t>())
                cfg.ro_port = static_cast<uint16_t>(*v);

            if (auto v = tbl["log"]["level"].value<std::string>()) cfg.log_level = *v;
            if (auto v = tbl["log"]["to_file"].value<bool>()) cfg.log_to_file = *v;
            if (auto v = tbl["log"]["file"].value<std::string>()) cfg.log_file = *v;
            if (auto v = tbl["log"]["max_files"].value<int64_t>())
                cfg.log_max_files = static_cast<int>(*v);
            if (auto v = tbl["log"]["max_size_bytes"].value<int64_t>())
                cfg.log_max_size_bytes = static_cast<size_t>(*v);
        }
        else
        {
            cfg.loaded_from = "(defaults)";
        }
    }
    catch (const std::exception& e)
    {
        cfg.loaded_from = "(parse error, using defaults)";
    }

    if (auto s = getenv_str("ARKAN_POSEIDON_SERVICE_NAME"); !s.empty()) cfg.service_name = s;
    if (auto s = getenv_str("ARKAN_POSEIDON_VERSION"); !s.empty()) cfg.version = s;
    if (auto s = getenv_str("ARKAN_POSEIDON_DEBUG"); !s.empty())
        cfg.debug = (to_lower(s) == "1" || to_lower(s) == "true");

    if (auto s = getenv_str("ARKAN_POSEIDON_QUERY_HOST"); !s.empty()) cfg.query_host = s;
    if (auto s = getenv_str("ARKAN_POSEIDON_QUERY_PORT"); !s.empty())
        cfg.query_port = static_cast<uint16_t>(std::stoi(s));
    if (auto s = getenv_str("ARKAN_POSEIDON_RO_HOST"); !s.empty()) cfg.ro_host = s;
    if (auto s = getenv_str("ARKAN_POSEIDON_RO_PORT"); !s.empty())
        cfg.ro_port = static_cast<uint16_t>(std::stoi(s));

    if (auto s = getenv_str("ARKAN_POSEIDON_LOG_LEVEL"); !s.empty()) cfg.log_level = s;
    if (auto s = getenv_str("ARKAN_POSEIDON_LOG_TO_FILE"); !s.empty())
        cfg.log_to_file = (to_lower(s) == "1" || to_lower(s) == "true");
    if (auto s = getenv_str("ARKAN_POSEIDON_LOG_FILE"); !s.empty()) cfg.log_file = s;
    if (auto s = getenv_str("ARKAN_POSEIDON_LOG_MAX_FILES"); !s.empty())
        cfg.log_max_files = std::max(1, std::stoi(s));
    if (auto s = getenv_str("ARKAN_POSEIDON_LOG_MAX_SIZE_BYTES"); !s.empty())
        cfg.log_max_size_bytes = static_cast<size_t>(std::stoll(s));

    return cfg;
}
