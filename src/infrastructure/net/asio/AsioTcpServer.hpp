#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "AsioTypes.hpp"
#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ITcpServer.hpp"

namespace arkan::thanatos::infrastructure::net::asio_impl
{

namespace ports = arkan::thanatos::application::ports::net;

/**
 * Main factory
 */
std::unique_ptr<ports::ITcpServer> MakeTcpServer(asio::io_context& io, std::uint16_t port,
                                                 std::shared_ptr<ports::IConnectionHandler> handler,
                                                 const std::string& bind_ip);

/**
 * Convenience overload: bind to "0.0.0.0" (all interfaces).
 * Kept for backward compatibility.
 */
std::unique_ptr<ports::ITcpServer> MakeTcpServer(
    asio::io_context& io, std::uint16_t port, std::shared_ptr<ports::IConnectionHandler> handler);

}  // namespace arkan::thanatos::infrastructure::net::asio_impl
