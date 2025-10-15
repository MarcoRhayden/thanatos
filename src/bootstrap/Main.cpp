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
#include "infrastructure/presentation/StartupSummary.hpp"
#include "infrastructure/terminal/Console.hpp"
#include "interface/ragnarok/RagnarokServer.hpp"
#include "shared/BannerPrinter.hpp"
#include "shared/BuildInfo.hpp"

namespace cfg = arkan::thanatos::infrastructure::config;
namespace logi = arkan::thanatos::infrastructure::log;

using arkan::thanatos::application::state::SessionRegistry;
using arkan::thanatos::infrastructure::presentation::print_startup_summary;
using arkan::thanatos::infrastructure::presentation::StartupSummary;
using arkan::thanatos::infrastructure::terminal::enable_vt_sequences_if_possible;
using arkan::thanatos::infrastructure::terminal::locale_is_utf8;
using arkan::thanatos::interface::ro::RagnarokServer;

// Global stop flags for signal handling.
// シグナル処理用の停止フラグ。
static std::atomic<bool> g_shutdown{false};
static boost::asio::io_context* g_io_ptr = nullptr;

static void HandleSignal(int)
{
    g_shutdown.store(true, std::memory_order_relaxed);
    if (g_io_ptr) g_io_ptr->stop();
}

// Tiny CLI: return path after `--config` (or default).
// 簡易CLI: `--config` の次の引数を返す（なければ既定）。
static std::string GetConfigPathFromArgs(int argc, char* argv[])
{
    for (int i = 1; i + 1 < argc; ++i)
        if (std::string(argv[i]) == "--config") return argv[i + 1];
    return "config/thanatos.toml";
}

int main(int argc, char* argv[])
try
{
    // Enable VT sequences on Windows consoles (no-op on POSIX).
    // Windowsの端末でVTシーケンスを有効化（POSIXでは何もしない）。
    enable_vt_sequences_if_possible();

    // ── Load config / 設定の読み込み ────────────────────────────────────────
    const std::string cfgPath = GetConfigPathFromArgs(argc, argv);
    cfg::Config config = cfg::LoadConfig(cfgPath);

    // ── Init logging / ログ初期化 ───────────────────────────────────────────
    logi::Logger::setConfig(config);
    logi::Logger::init(config.service_name, config.log_level, config.log_to_file, config.log_file,
                       config.log_max_size_bytes, config.log_max_files);

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(200ms);

    // Console capability & color policy.
    // 端末ケイパビリティとカラー方針。
    const bool utf8 = locale_is_utf8();
    const bool color = true && utf8;

    // Signature card: render once and print as-is.
    // シグネチャカード: 一度描画してそのまま出力。
    auto [sig_blob, sig_inner] =
        arkan::thanatos::shared::build_signature_with_width(/*color*/ color,
                                                            /*utf8Box*/ utf8);
#if defined(_WIN32)
    // Use the portable printer on Windows (UTF-16 under the hood).
    // Windowsでは移植性の高いプリンタを使用（内部でUTF-16）。
    arkan::thanatos::shared::print_utf8_banner(sig_blob);
#else
    std::fwrite(sig_blob.data(), 1, sig_blob.size(), stdout);
    std::fflush(stdout);
#endif

    // ── IO + registry ───────────────────────────────────────────────────────
    boost::asio::io_context io;
    g_io_ptr = &io;  // for signal handler / シグナルハンドラ用

    auto registry = std::make_shared<SessionRegistry>();

    // ── Servers ─────────────────────────────────────────────────────────────
    auto ro = std::make_shared<RagnarokServer>(io, registry, config);
    ro->start();

    // If we have an active binding, show the startup summary card.
    // バインドが有効なら起動サマリカードを表示。
    if (auto b = ro->active_binding())
    {
        StartupSummary sum{/*ro_host*/ b->ro_host,
                           /*query_host*/ b->query_host,
                           /*login*/ b->login_port,
                           /*char*/ b->char_port,
                           /*query*/ b->query_port,
                           /*set*/ b->set_index};

        // Align to the same inner width as the signature card for visual cohesion.
        // 見た目の統一感のため、シグネチャカードと同じ内部幅に合わせる。
        print_startup_summary(sum, /*color*/ color, /*utf8Box*/ utf8,
                              /*align_to_inner_cols*/ sig_inner);
    }

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
