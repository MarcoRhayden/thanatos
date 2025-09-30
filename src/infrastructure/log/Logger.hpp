#pragma once

#include <string_view>

#include "infrastructure/config/Config.hpp"

namespace arkan::poseidon::infrastructure::log
{

namespace apc = arkan::poseidon::infrastructure::config;

enum class LogStyle
{
    Default,
    Unformatted
};

#define LOG_UNFORMATTED ::arkan::poseidon::infrastructure::log::LogStyle::Unformatted
#define LOG_DEFAULT ::arkan::poseidon::infrastructure::log::LogStyle::Default

class Logger
{
   public:
    static void init(const std::string& service_name, const std::string& level, bool to_file,
                     const std::string& file_path, size_t max_size_bytes, int max_files);

    static void info(std::string_view msg, LogStyle style = LogStyle::Default);
    static void warn(std::string_view msg, LogStyle style = LogStyle::Default);
    static void debug(std::string_view msg, LogStyle style = LogStyle::Default);
    static void error(std::string_view msg, LogStyle style = LogStyle::Default);

    static void setConfig(const apc::Config& cfg);

   private:
    static apc::Config cfg_;
};

}  // namespace arkan::poseidon::infrastructure::log
