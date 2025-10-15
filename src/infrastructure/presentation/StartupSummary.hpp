#pragma once

#include <cstdint>
#include <string>

namespace arkan::thanatos::infrastructure::presentation
{

// Data used to render the card.
struct StartupSummary
{
    std::string ro_host;
    std::string query_host;
    std::uint16_t login_port{};
    std::uint16_t char_port{};
    std::uint16_t query_port{};
    std::size_t set_index{};
};

// Render the startup card.
void print_startup_summary(const StartupSummary& s, bool color = true, bool utf8Box = true,
                           int align_to_inner_cols = -1);

}  // namespace arkan::thanatos::infrastructure::presentation
