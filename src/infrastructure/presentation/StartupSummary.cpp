#include "infrastructure/presentation/StartupSummary.hpp"

#include <algorithm>
#include <string>
#include <vector>

#include "infrastructure/presentation/CardBuilder.hpp"
#include "infrastructure/terminal/Output.hpp"
#include "shared/terminal/Ansi.hpp"
#include "shared/terminal/TextWidth.hpp"

// Defuse possible Windows max/min macros.
// Windows の max/min マクロ衝突を回避。
#if defined(_WIN32)
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif

namespace arkan::thanatos::infrastructure::presentation
{
namespace ansi = arkan::thanatos::shared::ansi;
namespace term = arkan::thanatos::shared::terminal;
using arkan::thanatos::infrastructure::terminal::emit_blank_line;
using arkan::thanatos::infrastructure::terminal::emit_utf8_line;

// Visible width helper (terminal columns).
// 端末上の見かけ幅を取得。
static inline int vis_cols(const std::string& s)
{
    return static_cast<int>(term::visible_width_utf8(term::strip_ansi(s)));
}

void print_startup_summary(const StartupSummary& s, bool color, bool utf8Box,
                           int align_to_inner_cols)
{
    // Color tokens (empty when color == false).
    // 色トークン（color=false の時は空文字）。
    const std::string BOLD = color ? std::string(ansi::bold) : "";
    const std::string DIM = color ? std::string(ansi::dim) : "";
    const std::string RED = color ? std::string(ansi::red) : "";
    const std::string GREEN = color ? std::string(ansi::green) : "";
    const std::string CYAN = color ? std::string(ansi::cyan) : "";
    const std::string YELLOW = color ? std::string(ansi::yellow) : "";
    const std::string RESET = color ? std::string(ansi::reset) : "";

    auto ep = [](const std::string& h, std::uint16_t p) { return h + ":" + std::to_string(p); };

    // ── Content (no borders) / 罫線なしの本文 ───────────────────────────────
    const std::string title_text = RED + "Thanatos" + GREEN + " Online" + RESET;

    const std::string idx = DIM +
                            (CYAN + "Selected port set index" + RESET + " : " + BOLD +
                             std::to_string(s.set_index) + RESET) +
                            RESET;

    // Left labels with alignment.
    // 左側ラベルの整列。
    const std::string L1 = CYAN + "RO Login" + RESET;
    const std::string L2 = CYAN + "RO Char " + RESET;
    const std::string L3 = CYAN + "Query BUS" + RESET;

    const int label_w = (std::max)(vis_cols(L1), (std::max)(vis_cols(L2), vis_cols(L3)));
    auto pad_label = [&](const std::string& lbl)
    {
        const int pad = (std::max)(0, label_w - vis_cols(lbl));
        return lbl + std::string(static_cast<size_t>(pad), ' ');
    };

    const std::string login = pad_label(L1) + " : " + YELLOW + ep(s.ro_host, s.login_port) + RESET;
    const std::string ch = pad_label(L2) + " : " + YELLOW + ep(s.ro_host, s.char_port) + RESET;
    const std::string bus = pad_label(L3) + " : " + YELLOW + ep(s.query_host, s.query_port) + RESET;

    // We intentionally omit “GG Bridge” to keep the card compact and same height as the top banner.
    // 上段バナーと高さを揃えるため “GG Bridge” 行はあえて省略。

    // ── Width calculation / 幅計算 ─────────────────────────────────────────
    std::vector<std::string> content_lines{title_text, idx, login, ch, bus};
    int inner = max_visible_width(content_lines);  // computed by helper
    if (align_to_inner_cols > 0) inner = (std::max)(inner, align_to_inner_cols);

    // ── Build themed card / テーマ付きカードを構築 ─────────────────────────
    BannerTheme theme;
    theme.color = color;
    theme.utf8_box = utf8Box;
    theme.pad_lr = 2;
    theme.accent = color ? std::string(ansi::green) : std::string();
    theme.reset = color ? std::string(ansi::reset) : std::string();

    CardBuilder card(theme, inner);

    // ── Output (no blank line before) / 出力（前に空行なし） ────────────────
    emit_utf8_line(card.top());
    emit_utf8_line(card.centered_title(title_text));
    emit_utf8_line(card.sep());

    emit_utf8_line(card.wrap(card.pad(idx)));
    emit_utf8_line(card.sep());

    emit_utf8_line(card.wrap(card.pad(login)));
    emit_utf8_line(card.wrap(card.pad(ch)));
    emit_utf8_line(card.wrap(card.pad(bus)));

    emit_utf8_line(card.bot());
    emit_blank_line();
}

}  // namespace arkan::thanatos::infrastructure::presentation
