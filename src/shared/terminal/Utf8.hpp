#pragma once

#include <string>
#include <string_view>

namespace arkan::thanatos::shared::terminal
{
// Repeat a UTF-8 glyph n times.
// UTF-8グリフをn回繰り返す。
inline std::string repeat_utf8(std::string_view g, size_t n)
{
    std::string out;
    out.reserve(g.size() * n);
    for (size_t i = 0; i < n; ++i) out += g;
    return out;
}
}  // namespace arkan::thanatos::shared::terminal
