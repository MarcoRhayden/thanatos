#include "Logger.hpp"

#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <vector>

namespace fs = std::filesystem;
namespace arkan::poseidon::infrastructure::log
{

namespace
{
std::shared_ptr<spdlog::logger> g_logger;

spdlog::level::level_enum parse_level(const std::string& s)
{
    auto l = spdlog::level::from_str(s);
    if (l == spdlog::level::off && s != "off") return spdlog::level::info;
    return l;
}
}  // namespace

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
        spdlog::info("Logger initialized (level={}, file_sink={})", level, to_file ? "on" : "off");
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        spdlog::shutdown();
        auto fallback = spdlog::stdout_color_mt("fallback");
        fallback->set_level(spdlog::level::debug);
        fallback->warn("Failed to init spdlog: {}", ex.what());
    }
}

void Logger::info(const std::string& msg)
{
    spdlog::info(msg);
}
void Logger::warn(const std::string& msg)
{
    spdlog::warn(msg);
}
void Logger::error(const std::string& msg)
{
    spdlog::error(msg);
}

}  // namespace arkan::poseidon::infrastructure::log
