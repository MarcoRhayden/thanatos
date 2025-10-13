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

// ──────────────────────────────────────────────────────────────────────────────
//  ⚔ Thanatos – Bootstrap (composition root)
//  Application entrypoint. Wires configuration, logging and servers.
//  アプリのエントリポイント。設定・ロギング・サーバの初期化を行う。
// ──────────────────────────────────────────────────────────────────────────────

#include <atomic>
#include <boost/asio.hpp>
#include <chrono>
#include <csignal>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "application/state/SessionRegistry.hpp"
#include "infrastructure/config/Config.hpp"
#include "infrastructure/log/Logger.hpp"
#include "interface/ragnarok/RagnarokServer.hpp"
#include "shared/BannerPrinter.hpp"
#include "shared/BuildInfo.hpp"

namespace cfg = arkan::thanatos::infrastructure::config;
namespace logi = arkan::thanatos::infrastructure::log;
using arkan::thanatos::application::state::SessionRegistry;
using arkan::thanatos::interface::ro::RagnarokServer;

// ── Windows console UTF‑8 helper / Windows コンソール UTF‑8 設定 ───────────────
#if defined(_WIN32)
static void SetupWinConsoleUtf8()
{
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    if (HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE))
    {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode))
        {
            mode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, mode);
        }
    }
}
#endif

static std::atomic<bool> g_shutdown{false};
static boost::asio::io_context* g_io_ptr = nullptr;

static void HandleSignal(int)
{
    g_shutdown.store(true, std::memory_order_relaxed);
    if (g_io_ptr) g_io_ptr->stop();
}

// ── Tiny CLI: get --config <path> / 簡易 CLI: --config <path> ────────────────
static std::string GetConfigPathFromArgs(int argc, char* argv[])
{
    for (int i = 1; i + 1 < argc; ++i)
    {
        if (std::string(argv[i]) == "--config")
        {
            return argv[i + 1];
        }
    }
    return "config/thanatos.toml";  // default
}

int main(int argc, char* argv[])
try
{
#if defined(_WIN32)
    SetupWinConsoleUtf8();
#endif

    // ── Load config / 設定の読み込み ────────────────────────────────────────
    const std::string cfgPath = GetConfigPathFromArgs(argc, argv);
    cfg::Config config = cfg::LoadConfig(cfgPath);

    // ── Init logging / ログ初期化 ───────────────────────────────────────────
    logi::Logger::setConfig(config);
    logi::Logger::init(config.service_name, config.log_level, config.log_to_file, config.log_file,
                       config.log_max_size_bytes, config.log_max_files);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(200ms);

    // ── Splash: print banner directly to stdout (robust to consoles) ────────
    arkan::thanatos::shared::print_banner_or_fallback(
        std::string(arkan::thanatos::shared::kSignature));
    arkan::thanatos::shared::print_banner_or_fallback(
        std::string(arkan::thanatos::shared::kSignatureFooter));

    // Linhas informativas em log (UTF‑8 puro em arquivo)
    logi::Logger::info("Service: " + config.service_name);
    logi::Logger::info("Version: " + config.version);
    logi::Logger::info(std::string("Profile: ") +
                       std::string(arkan::thanatos::shared::kBuildProfile));
    logi::Logger::info("Config:  " + config.loaded_from);

    // ── IO + registry ───────────────────────────────────────────────────────
    boost::asio::io_context io;
    g_io_ptr = &io;  // for signal handler

    auto registry = std::make_shared<SessionRegistry>();

    // ── Servers ─────────────────────────────────────────────────────────────
    auto ro = std::make_shared<RagnarokServer>(io, registry, config);
    ro->start();

    auto ep = [](const std::string& h, uint16_t p) { return h + ":" + std::to_string(p); };
    logi::Logger::info("RO host=" + config.ro_host +
                       " | Login=" + ep(config.ro_host, config.login_port) +
                       " | Query=" + ep(config.query_host, config.query_port));

    // ── Signals ─────────────────────────────────────────────────────────────
    std::signal(SIGINT, HandleSignal);
    std::signal(SIGTERM, HandleSignal);

    // ── Run ────────────────────────────────────────────────────────────────
    io.run();

    // ── Graceful stop ──────────────────────────────────────────────────────
    ro->stop();
    logi::Logger::info("Thanatos stopped. Bye.");
    return 0;
}
catch (const std::exception& e)
{
    logi::Logger::error(std::string("Fatal error: ") + e.what());
    return 1;
}
catch (...)
{
    logi::Logger::error("Fatal error: unknown exception");
    return 2;
}
