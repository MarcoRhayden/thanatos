#include <chrono>
#include <iostream>
#include <thread>

#include "infrastructure/log/Logger.hpp"
#include "shared/BuildInfo.hpp"

using arkan::poseidon::infrastructure::log::Logger;
using namespace std::chrono_literals;

int main()
{
    Logger::init(std::string(arkan::poseidon::shared::kProjectName));

    Logger::info("===========================================");
    Logger::info(std::string("Service: ") + std::string(arkan::poseidon::shared::kProjectName));
    Logger::info(std::string("Version: ") + std::string(arkan::poseidon::shared::kVersion));
    Logger::info(std::string("Profile: ") + std::string(arkan::poseidon::shared::kBuildProfile));
    Logger::info("===========================================");

    Logger::info("Arkan-Poseidon up. (Skeleton running)");
    for (;;)
    {
        std::this_thread::sleep_for(50ms);
    }

    return 0;
}
