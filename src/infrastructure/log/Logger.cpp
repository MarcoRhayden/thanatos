#include "Logger.hpp"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <vector>

namespace arkan::poseidon::infrastructure::log
{

namespace
{
std::shared_ptr<spdlog::logger> g_logger;
}

void Logger::init(const std::string& service_name, bool console_only, const std::string& file_path)
{
    try
    {
        std::vector<spdlog::sink_ptr> sinks;
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
        sinks.push_back(console_sink);

        if (!console_only && !file_path.empty())
        {
            auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(file_path, true);
            file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
            sinks.push_back(file_sink);
        }

        g_logger = std::make_shared<spdlog::logger>(service_name, sinks.begin(), sinks.end());
#if defined(NDEBUG)
        g_logger->set_level(spdlog::level::info);
#else
        g_logger->set_level(spdlog::level::debug);
#endif
        spdlog::set_default_logger(g_logger);
        spdlog::info("Logger initialized for {}", service_name);
    }
    catch (const spdlog::spdlog_ex& ex)
    {
        // stdout fallback
        spdlog::shutdown();
        auto fallback = spdlog::stdout_color_mt("fallback");
        fallback->set_level(spdlog::level::debug);
        fallback->warn("Failed to init spdlog sinks: {}", ex.what());
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
