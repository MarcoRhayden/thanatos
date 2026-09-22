#pragma once

#include <chrono>

namespace arkan::thanatos::infrastructure::net
{

// Shared write watchdog used by Asio TCP client and server sessions.
inline constexpr auto kWriteTimeout = std::chrono::seconds{30};

}  // namespace arkan::thanatos::infrastructure::net
