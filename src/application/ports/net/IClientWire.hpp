#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace arkan::thanatos::application::ports::net
{

// Minimal abstraction for sending raw bytes to the active RO client.
struct IClientWire
{
    virtual ~IClientWire() = default;

    // Sends raw bytes to the RO client socket.
    // Returns true if the write was queued/accepted.
    virtual bool send_to_client(const std::vector<std::uint8_t>& bytes) = 0;
};

}  // namespace arkan::thanatos::application::ports::net
