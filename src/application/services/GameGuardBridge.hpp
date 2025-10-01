#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include "application/ports/net/IClientWire.hpp"
#include "application/ports/query/IQueryServer.hpp"

namespace arkan::thanatos::application::services
{

// Orchestrates a GameGuard round-trip:
// - Receives ThanatosQuery (GG challenge) from IQueryServer -> forwards to the RO client via
// IClientWire.
// - Observes the RO client's RX, the first packet within the window becomes a ThanatosReply to the
// IQueryServer.
class GameGuardBridge
{
   public:
    using Clock = std::chrono::steady_clock;

    // query: port for the Thanatos channel (adapter in the interface layer).
    explicit GameGuardBridge(ports::query::IQueryServer& query);

    // Inject/update the active session wire (done by RagnarokServer when the session changes).
    void bindClientWire(ports::net::IClientWire* wire);

    // RX Tap: Call this in the RO client receive pipeline.
    void onClientPacket(const std::uint8_t* data, std::size_t len);

    // Configurable (can expose via setters by reading thanatos.toml if desired)
    void set_timeout(std::chrono::milliseconds ms)
    {
        timeout_ = ms;
    }
    void set_size_bounds(std::size_t min, std::size_t max)
    {
        min_len_ = min;
        max_len_ = max;
    }

   private:
    ports::query::IQueryServer& query_;
    ports::net::IClientWire* wire_{nullptr};

    bool pending_{false};
    Clock::time_point deadline_{};
    std::size_t min_len_ = 2;
    std::size_t max_len_ = 2048;
    std::chrono::milliseconds timeout_{1500};

    // Callback registered in IQueryServer
    void on_query_from_kore_(std::vector<std::uint8_t> gg_query);
};

}  // namespace arkan::thanatos::application::services
