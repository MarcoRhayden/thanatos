#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

#include "application/ports/net/IClientWire.hpp"
#include "application/ports/query/IQueryServer.hpp"

namespace arkan::thanatos::application::services
{

class GameGuardBridge
{
   public:
    using Clock = std::chrono::steady_clock;

    enum class GGStrategy
    {
        FULL_FRAME,     // Send full 09D0 frame (RagnarokServer.pm behavior)
                        // 09D0 を丸ごと返す（Perl RagnarokServer.pm と同様）
        BODY_TRUNC_18,  // Send only the body (exclude 4B header), truncated to 18 bytes
                        // (EmbedServer.pm hotfix) 先頭4Bを除く本体のみ・18Bに切詰
        BODY_LEN,       // Send only the body using internal length (diagnostic)
                        // 本体のみを内部長で返す（診断用）
        AUTO            // Choose strategy by last 09CF size (72->TRUNC, 80->FULL)
                        // 直前の09CFサイズで自動選択（72→TRUNC、80→FULL）
    };

    explicit GameGuardBridge(ports::query::IQueryServer& query);

    void bindClientWire(ports::net::IClientWire* wire);

    // Intercepts client->server frames. Only reacts to 0x09D0 (optionally 0x099F).
    // クライアント→サーバのフレームを傍受。0x09D0（必要なら 0x099F）のみ処理。
    bool maybe_consume_c2s(const std::uint8_t* data, std::size_t len);

    void set_timeout(std::chrono::milliseconds ms)
    {
        base_timeout_ = ms;
        timeout_ = ms;
    }

    void set_size_bounds(std::size_t min, std::size_t max)
    {
        min_len_ = min;
        max_len_ = max;
    }

    void set_greedy_window(std::chrono::milliseconds ms)
    {
        greedy_window_ = ms;
    }

    void set_strategy(GGStrategy s)
    {
        strategy_ = s;
    }

    void set_max_retries(int max_retries)
    {
        max_retries_ = max_retries;
    }

    void set_timeout_randomization(bool enabled, int min_percent = 80, int max_percent = 120)
    {
        randomize_timeout_ = enabled;
        timeout_min_percent_ = min_percent;
        timeout_max_percent_ = max_percent;
    }

   private:
    ports::query::IQueryServer& query_;
    ports::net::IClientWire* wire_{nullptr};

    bool pending_{false};
    Clock::time_point deadline_{};
    Clock::time_point sent_at_{};
    std::size_t min_len_ = 2;
    std::size_t max_len_ = 2048;
    std::chrono::milliseconds timeout_{1500};
    std::chrono::milliseconds greedy_window_{200};

    GGStrategy strategy_{GGStrategy::FULL_FRAME};  // default
    std::size_t last_gg_request_len_{0};           // last 09CF total length

    // Retry and randomization
    int retry_count_{0};                       // Attempt counter
    int max_retries_{3};                       // Maximum attempts (default: 3)
    std::vector<std::uint8_t> last_gg_query_;  // Last packet sent (for retry)

    std::chrono::milliseconds base_timeout_{1500};  // Timeout base (no randomization)
    bool randomize_timeout_{false};                 // Whether to randomize the timeout
    int timeout_min_percent_{80};                   // Minimum: 80% of base timeout
    int timeout_max_percent_{120};                  // Maximum: 120% of base timeout

    std::mt19937 rng_{std::random_device{}()};  // Random number generator

    void on_query_from_kore_(std::vector<std::uint8_t> gg_query);

    std::chrono::milliseconds get_randomized_timeout();
};

}  // namespace arkan::thanatos::application::services