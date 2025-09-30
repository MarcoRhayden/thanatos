/*
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⡀⠀⠀⠀⢀⣀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣤⣶⣾⣿⡉⢤⣍⡓⢶⣶⣦⣤⣉⠒⠤⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⣿⣿⣿⣿⣿⣿⣷⡀⠙⣿⣷⣌⠻⣿⣿⣿⣶⣌⢳⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⠀⣰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣄⠈⢿⣿⡆⠹⣿⣿⣿⣿⣷⣿⡀⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⠀⣰⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣷⣄⠹⣿⡄⢻⣿⣿⣿⣿⣿⣧⠀⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⢠⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠿⠿⣿⣿⣷⣽⣷⢸⣿⡿⣿⡿⠿⠿⣆⠀⠀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⣼⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡄⠀⠀⠀⠐⠾⢭⣭⡼⠟⠃⣤⡆⠘⢟⢺⣦⡀⠀⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⢛⣥⣶⠾⠿⠛⠳⠶⢬⡁⠀⠀⠘⣃⠤⠤⠤⢍⠻⡄⠀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⡿⣫⣾⡿⢋⣥⣶⣿⠿⢿⣿⣿⣿⠩⠭⢽⣷⡾⢿⣿⣦⢱⡹⡀⠀⠀⠀
⠀⠀⠀⠀⠀⠀⣿⣿⣿⣿⣿⣿⡟⠈⠛⠏⠰⢿⣿⣿⣧⣤⣼⣿⣿⣿⡏⠩⠽⣿⣀⣼⣿⣿⢻⣷⢡⠀⠀⠀
⠀⠀⠀⠀⠀⢰⣿⣿⣿⣿⣿⣿⢁⣿⣷⣦⡀⠀⠉⠙⠛⠛⠛⠋⠁⠙⢻⡆⠀⢌⣉⠉⠉⠀⠸⣿⣇⠆⠀⠀
⠀⠀⠀⠀⢀⣾⣿⣿⣿⣿⣿⡇⢸⣿⣿⣿⣿⠷⣄⢠⣶⣾⣿⣿⣿⣿⣿⠁⠀⠀⢿⣿⣿⣿⣷⠈⣿⠸⡀⠀
⠀⠀⠀⠀⣼⣿⣿⣿⣿⣿⣿⠀⣌⡛⠿⣿⣿⠀⠈⢧⢿⣿⡿⠟⠋⠉⣠⣤⣤⣤⣄⠙⢿⣿⠏⣰⣿⡇⢇⠀
⠀⠀⠀⣼⣿⣿⣿⣿⣿⣿⡇⢸⣿⣿⣶⣤⣙⠣⢀⠈⠘⠏⠀⠀⢀⣴⢹⡏⢻⡏⣿⣷⣄⠉⢸⣿⣿⣷⠸⡄
⠀⠀⣸⣿⣿⣿⣿⣿⣿⣿⠁⣾⣟⣛⠛⠛⠻⠿⠶⠬⠔⠀⣠⡶⠋⠿⠈⠷⠸⠇⠻⠏⠻⠆⣀⢿⣿⣿⡄⢇
⠀⢰⣿⣿⣿⣿⠿⠿⠛⠋⣰⣿⣿⣿⣿⠿⠿⠿⠒⠒⠂⠀⢨⡤⢶⣶⣶⣶⣶⣶⣶⣶⣶⠆⠃⣀⣿⣿⡇⣸
⢀⣿⣿⠿⠋⣡⣤⣶⣾⣿⣿⣿⡟⠁⠀⣠⣤⣴⣶⣶⣾⣿⣿⣷⡈⢿⣿⣿⣿⣿⠿⠛⣡⣴⣿⣿⣿⣿⠟⠁
⣼⠋⢁⣴⣿⣿⣿⣿⣿⣿⣿⣿⠀⠀⠀⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣎⠻⠟⠋⣠⣴⣿⣿⣿⣿⠿⠋⠁⠀⠀
⢿⣷⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⠀⣴⠀⠻⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣧⣠⣾⣿⠿⠿⠟⠋⠁⠀⠀⠀⠀⠀
⠀⠉⠛⠛⠿⠿⠿⢿⣿⣿⣿⣵⣾⣿⣧⣾⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⣿⡿⠏⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀
*/

//
//  ⚔ Forged in chaos. Delivered with precision.
//
//  Author : Rhayden - Arkan Software
//  Email  : Rhayden@arkansoftware.com
//

#include <boost/asio.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

#include <chrono>
#include <thread>

#include "application/state/SessionRegistry.hpp"
#include "infrastructure/config/Config.hpp"
#include "infrastructure/log/Logger.hpp"
#include "infrastructure/net/asio/AsioTcpServer.hpp"
#include "interface/ragnarok/RagnarokServer.hpp"
#include "shared/BuildInfo.hpp"

namespace cfg = arkan::poseidon::infrastructure::config;
namespace asio_impl = arkan::poseidon::infrastructure::net::asio_impl;
using arkan::poseidon::infrastructure::log::Logger;

#ifdef _WIN32
static void SetupWinConsoleUtf8()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    if (GetConsoleMode(hOut, &mode))
    {
        mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
        SetConsoleMode(hOut, mode);
    }
}
#endif

namespace cfg = arkan::poseidon::infrastructure::config;
namespace asio_impl = arkan::poseidon::infrastructure::net::asio_impl;
using arkan::poseidon::infrastructure::log::Logger;

int main()
{
#ifdef _WIN32
    SetupWinConsoleUtf8();
#endif

    auto config = cfg::LoadConfig("config/poseidon.toml");
    Logger::setConfig(config);
    Logger::init(config.service_name, config.log_level, config.log_to_file, config.log_file,
                 config.log_max_size_bytes, config.log_max_files);

    // Splash + metadata
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    Logger::info(std::string(arkan::poseidon::shared::kSignature), LOG_UNFORMATTED);
    Logger::info(std::string(arkan::poseidon::shared::kSignatureFooter), LOG_UNFORMATTED);
    Logger::info(std::string("Service: ") + config.service_name);
    Logger::info(std::string("Version: ") + config.version);
    Logger::info(std::string("Profile: ") + std::string(arkan::poseidon::shared::kBuildProfile));
    Logger::info(std::string("Config:  ") + config.loaded_from);

    // IO and registers
    boost::asio::io_context io;
    auto registry = std::make_shared<arkan::poseidon::application::state::SessionRegistry>();

    // RagnarokServer
    auto ragnarok_server =
        std::make_shared<arkan::poseidon::interface::ro::RagnarokServer>(io, registry, config);
    ragnarok_server->start();

    Logger::info("Login on " + std::to_string(config.login_port) + ", Char on " +
                 std::to_string(config.char_port));

    io.run();
}
