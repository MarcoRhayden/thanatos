#pragma once

#include <cstdint>
#include <string>

namespace arkan::thanatos::infrastructure::presentation
{

// Startup banner data / 起動サマリ用データ
struct StartupSummary
{
    std::string ro_host;
    std::string query_host;
    std::uint16_t login_port{};
    std::uint16_t char_port{};
    std::uint16_t query_port{};
    std::size_t set_index{};
    bool color{true};  // enable ANSI colors if console supports / コンソール対応時のみカラー
    bool prefer_utf8_box{false};  // try UTF-8 box-drawing (may be overridden) / UTF-8 罫線優先
};

// Print banner to console (robust; ASCII fallback on Win) / 起動サマリをコンソールへ出力（Win では
// ASCII を既定）
void print_startup_summary(const StartupSummary& s);

}  // namespace arkan::thanatos::infrastructure::presentation
