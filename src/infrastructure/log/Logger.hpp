#pragma once
#include <memory>
#include <string>

namespace arkan::poseidon::infrastructure::log
{

class Logger
{
   public:
    // Init global logger (thread-safe no spdlog)
    static void init(const std::string& service_name, bool console_only = true,
                     const std::string& file_path = "");

    // Conveniências
    static void info(const std::string& msg);
    static void warn(const std::string& msg);
    static void error(const std::string& msg);
};

}  // namespace arkan::poseidon::infrastructure::log
