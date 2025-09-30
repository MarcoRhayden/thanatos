#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace arkan::poseidon::application::ports::query
{

// Message types in the Poseidon channel (compatible with the PSDN bridge).
enum class MsgType : std::uint16_t
{
    PoseidonQuery = 0x0051,  // OpenKore -> Poseidon (payload = GG challenge)
    PoseidonReply = 0x0052   // Poseidon -> OpenKore (payload = GG reply from RO client)
};

// High-level port for the Query server (no network/Asio dependency).
struct IQueryServer
{
    virtual ~IQueryServer() = default;

    // Starts/stops listening on the query channel.
    virtual void start() = 0;
    virtual void stop() = 0;

    // Register callback for when PoseidonQuery (GG challenge) arrives.
    virtual void onQuery(std::function<void(std::vector<std::uint8_t>)> cb) = 0;

    // Send PoseidonReply back to the query producer (e.g. OpenKore).
    virtual void sendReply(const std::vector<std::uint8_t>& gg_reply) = 0;

    // (Optional) Useful identification for logs.
    virtual std::string endpoint_description() const = 0;
};

}  // namespace arkan::poseidon::application::ports::query
