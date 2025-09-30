#include "application/services/GameGuardBridge.hpp"

#include <algorithm>
#include <chrono>
#include <string>

#include "infrastructure/log/Logger.hpp"

using arkan::poseidon::infrastructure::log::Logger;

namespace arkan::poseidon::application::services
{

GameGuardBridge::GameGuardBridge(ports::query::IQueryServer& query) : query_(query)
{
    Logger::debug("[gg] bridge constructed, wiring onQuery()");

    // Register the Query callback (GG challenge coming from OpenKore).
    query_.onQuery(
        [this](std::vector<std::uint8_t> gg_query)
        {
            const char* pend = pending_ ? "true" : "false";
            Logger::debug(std::string("[gg] kore -> bridge: query len=") +
                          std::to_string(gg_query.size()) + " (pending=" + pend + ")");
            this->on_query_from_kore_(std::move(gg_query));
        });
}

void GameGuardBridge::bindClientWire(ports::net::IClientWire* wire)
{
    wire_ = wire;
    Logger::debug("[gg] bound client wire");
}

void GameGuardBridge::on_query_from_kore_(std::vector<std::uint8_t> gg_query)
{
    if (pending_)
    {
        Logger::debug("[gg] dropping kore query (pending already true)");
        return;
    }
    if (!wire_)
    {
        Logger::debug("[gg] dropping kore query (no client wire bound)");
        return;
    }

    // Forwards GG challenge to the active RO client.
    if (!wire_->send_to_client(gg_query))
    {
        Logger::debug("[gg] failed to send gg_query to client");
        return;
    }

    pending_ = true;
    deadline_ = Clock::now() + timeout_;

    const auto deadline_ms = static_cast<long long>(
        std::chrono::duration_cast<std::chrono::milliseconds>(timeout_).count());

    Logger::debug(std::string("[gg] bridge -> client: forwarded gg_query len=") +
                  std::to_string(gg_query.size()) + ", deadline=" + std::to_string(deadline_ms) +
                  "ms");
}

void GameGuardBridge::onClientPacket(const std::uint8_t* data, std::size_t len)
{
    if (!pending_) return;

    if (Clock::now() > deadline_)
    {
        pending_ = false;
        Logger::debug("[gg] timeout waiting client reply");
        return;
    }

    // Minimal heuristic: use the first packet in the window.
    if (len >= min_len_ && len <= max_len_)
    {
        std::vector<std::uint8_t> reply(data, data + len);
        Logger::debug(std::string("[gg] client -> bridge: reply len=") + std::to_string(len) +
                      " (forwarding to kore)");
        query_.sendReply(reply);  // returns PoseidonReply
        pending_ = false;
        Logger::debug("[gg] state: pending=false");
    }
    else
    {
        Logger::debug(std::string("[gg] client packet ignored (len=") + std::to_string(len) +
                      " out of gg window [" + std::to_string(min_len_) + ".." +
                      std::to_string(max_len_) + "])");
    }
}

}  // namespace arkan::poseidon::application::services
