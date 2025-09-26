#pragma once

#include <cstdint>
#include <memory>

#include "AsioTypes.hpp"
#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ITcpServer.hpp"

namespace arkan::poseidon::infrastructure::net::asio_impl
{

namespace ports = arkan::poseidon::application::ports::net;

std::unique_ptr<ports::ITcpServer> MakeTcpServer(
    asio::io_context& io, std::uint16_t port, std::shared_ptr<ports::IConnectionHandler> handler);

}  // namespace arkan::poseidon::infrastructure::net::asio_impl
