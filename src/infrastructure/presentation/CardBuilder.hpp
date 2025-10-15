#pragma once

#include <algorithm>
#include <string>
#include <vector>

#include "shared/terminal/Ansi.hpp"
#include "shared/terminal/TextWidth.hpp"
#include "shared/terminal/Utf8.hpp"

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
namespace term = arkan::thanatos::shared::terminal;
namespace ansi = arkan::thanatos::shared::ansi;

// Simple theme shared by all cards.
// すべてのカードで共有する簡易テーマ。
struct BannerTheme
{
    bool color{true};
    bool utf8_box{true};
    int pad_lr{2};
    std::string accent = std::string(ansi::green);
    std::string reset = std::string(ansi::reset);
};

// Helper to render consistent framed cards.
// 一貫した枠付きカードを描画するヘルパー。
class CardBuilder
{
   public:
    CardBuilder(BannerTheme t, int inner_width) : theme_(std::move(t)), inner_(inner_width) {}

    // total width including inner padding (no side glyphs)
    // 内側パディング込みの実幅（左右罫線は含まない）
    int box_width() const
    {
        return inner_ + theme_.pad_lr * 2;
    }

    std::string top() const
    {
        const int bw = box_width();
        return theme_.utf8_box ? "┌" + term::repeat_utf8("─", bw) + "┐"
                               : "+" + std::string(static_cast<size_t>(bw), '-') + "+";
    }

    std::string sep() const
    {
        const int bw = box_width();
        return theme_.utf8_box ? "├" + term::repeat_utf8("─", bw) + "┤"
                               : "+" + std::string(static_cast<size_t>(bw), '-') + "+";
    }

    std::string bot() const
    {
        const int bw = box_width();
        return theme_.utf8_box ? "└" + term::repeat_utf8("─", bw) + "┘"
                               : "+" + std::string(static_cast<size_t>(bw), '-') + "+";
    }

    std::string wrap(const std::string& s) const
    {
        return (theme_.utf8_box ? "│" : "|") + s + (theme_.utf8_box ? "│" : "|");
    }

    std::string pad(const std::string& raw) const
    {
        const int vis = static_cast<int>(term::visible_width_utf8(term::strip_ansi(raw)));
        const int pad = (std::max)(0, inner_ - vis);
        return std::string(static_cast<size_t>(theme_.pad_lr), ' ') + raw +
               std::string(static_cast<size_t>(pad), ' ') +
               std::string(static_cast<size_t>(theme_.pad_lr), ' ');
    }

    std::string centered_title(const std::string& title) const
    {
        const int tw = static_cast<int>(term::visible_width_utf8(term::strip_ansi(title)));
        const int pad_total = (std::max)(0, inner_ - (tw + 2));  // spaces around title
        const int left = pad_total / 2;
        const int right = pad_total - left;

        const std::string leftFill = theme_.utf8_box ? term::repeat_utf8("─", left)
                                                     : std::string(static_cast<size_t>(left), '-');
        const std::string rightFill = theme_.utf8_box
                                          ? term::repeat_utf8("─", right)
                                          : std::string(static_cast<size_t>(right), '-');

        return wrap(std::string(static_cast<size_t>(theme_.pad_lr), ' ') + theme_.accent +
                    leftFill + theme_.reset + " " + title + " " + theme_.accent + rightFill +
                    theme_.reset + std::string(static_cast<size_t>(theme_.pad_lr), ' '));
    }

    int inner_width() const
    {
        return inner_;
    }

   private:
    BannerTheme theme_;
    int inner_;
};

// Compute max visible width among lines.
// 複数行の見かけ幅の最大値を算出。
inline int max_visible_width(const std::vector<std::string>& lines)
{
    int m = 0;
    for (const auto& s : lines)
    {
        const int w = static_cast<int>(term::visible_width_utf8(term::strip_ansi(s)));
        m = (std::max)(m, w);
    }
    return m;
}

}  // namespace arkan::thanatos::infrastructure::presentation
