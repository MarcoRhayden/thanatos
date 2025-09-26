#include <chrono>
#include <thread>

#include "infrastructure/config/Config.hpp"
#include "infrastructure/log/Logger.hpp"
#include "shared/BuildInfo.hpp"

using arkan::poseidon::infrastructure::log::Logger;
namespace cfg = arkan::poseidon::infrastructure::config;
using namespace std::chrono_literals;

int main()
{
    auto config = cfg::LoadConfig("config/poseidon.toml");

    Logger::init(config.service_name, config.log_level, config.log_to_file, config.log_file,
                 config.log_max_size_bytes, config.log_max_files);

    Logger::info("===========================================");
    Logger::info(std::string("Service: ") + config.service_name);
    Logger::info(std::string("Version: ") + config.version);
    Logger::info(std::string("Profile: ") + std::string(arkan::poseidon::shared::kBuildProfile));
    Logger::info(std::string("Config:  ") + config.loaded_from);
    Logger::info("===========================================");

    Logger::info("Skeleton running with config loaded.");
    for (;;) std::this_thread::sleep_for(100ms);
}
