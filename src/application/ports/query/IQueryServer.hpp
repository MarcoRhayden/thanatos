#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace arkan::thanatos::application::ports::query
{

// Message types in the thanatos channel (compatible with the PSDN bridge).
enum class MsgType : std::uint16_t
{
    ThanatosQuery = 0x0051,  // OpenKore -> thanatos (payload = GG challenge)
    ThanatosReply = 0x0052   // thanatos -> OpenKore (payload = GG reply from RO client)
};

// High-level port for the Query server (no network/Asio dependency).
struct IQueryServer
{
    virtual ~IQueryServer() = default;

    // Starts/stops listening on the query channel.
    virtual void start() = 0;
    virtual void stop() = 0;

    // Register callback for when ThanatosQuery (GG challenge) arrives.
    virtual void onQuery(std::function<void(std::vector<std::uint8_t>)> cb) = 0;

    // Send ThanatosReply back to the query producer (e.g. OpenKore).
    virtual void sendReply(const std::vector<std::uint8_t>& gg_reply) = 0;

    // (Optional) Useful identification for logs.
    virtual std::string endpoint_description() const = 0;
};

}  // namespace arkan::thanatos::application::ports::query
