#include "Logger.hpp"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <filesystem>
#include <memory>
#include <mutex>
#include <string_view>
#include <vector>

namespace fs = std::filesystem;
namespace arkan::poseidon::infrastructure::log
{

apc::Config Logger::cfg_{};

namespace
{
std::mutex g_plain_mutex;
std::shared_ptr<spdlog::logger> g_logger;

spdlog::level::level_enum parse_level(const std::string& s)
{
    auto l = spdlog::level::from_str(s);
    if (l == spdlog::level::off && s != "off") return spdlog::level::info;
    return l;
}

inline void print_unformatted(std::string_view msg)
{
    std::lock_guard<std::mutex> lk(g_plain_mutex);
    std::fwrite(msg.data(), 1, msg.size(), stdout);
    if (msg.empty() || msg.back() != '\n') std::fputc('\n', stdout);
    std::fflush(stdout);
}

inline bool is_unformatted(LogStyle s)
{
    return s == LogStyle::Unformatted;
}

}  // namespace

void Logger::setConfig(const apc::Config& cfg)
{
    cfg_ = cfg;
}

void Logger::init(const std::string& service_name, const std::string& level, bool to_file,
                  const std::string& file_path, size_t max_size_bytes, int max_files)
{
    try
    {
        std::vector<spdlog::sink_ptr> sinks;

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        sinks.push_back(console_sink);

        if (to_file && !file_path.empty())
        {
            try
            {
                fs::create_directories(fs::path(file_path).parent_path());
            }
            catch (...)
            {
            }
            auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
                file_path, max_size_bytes, max_files);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            sinks.push_back(file_sink);
        }

        g_logger = std::make_shared<spdlog::logger>(service_name, sinks.begin(), sinks.end());
        g_logger->set_level(parse_level(level));
        spdlog::set_default_logger(g_logger);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        spdlog::shutdown();
        auto fallback = spdlog::stdout_color_mt("fallback");
        fallback->set_level(spdlog::level::debug);
        fallback->warn("Failed to init spdlog: {}", ex.what());
    }
}

void Logger::info(std::string_view msg, LogStyle style)
{
    if (is_unformatted(style))
    {
        print_unformatted(msg);
        return;
    }
    spdlog::info("{}", msg);
}

void Logger::warn(std::string_view msg, LogStyle style)
{
    if (is_unformatted(style))
    {
        print_unformatted(msg);
        return;
    }
    spdlog::warn("{}", msg);
}

void Logger::debug(std::string_view msg, LogStyle style)
{
    if (is_unformatted(style))
    {
        print_unformatted(msg);
        return;
    }
    if (cfg_.debug) spdlog::debug("{}", msg);
}

void Logger::error(std::string_view msg, LogStyle style)
{
    if (is_unformatted(style))
    {
        print_unformatted(msg);
        return;
    }
    spdlog::error("{}", msg);
}

}  // namespace arkan::poseidon::infrastructure::log
