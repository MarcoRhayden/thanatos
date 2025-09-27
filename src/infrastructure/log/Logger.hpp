#pragma once
#include <string>

namespace arkan::poseidon::infrastructure::log
{

class Logger
{
   public:
    // Init global logger (thread-safe no spdlog)
    static void init(const std::string& service_name, const std::string& level, bool to_file,
                     const std::string& file_path, size_t max_size_bytes, int max_files);

    // Conveniências
    static void info(const std::string& msg);
    static void warn(const std::string& msg);
    static void error(const std::string& msg);
};

}  // namespace arkan::poseidon::infrastructure::log
