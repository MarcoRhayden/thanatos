#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <system_error>
#include <unordered_map>

#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "application/state/SessionRegistry.hpp"
#include "infrastructure/log/Logger.hpp"
#include "infrastructure/net/asio/AsioTcpClient.hpp"

namespace arkan::thanatos::interface::dev
{
namespace ports = arkan::thanatos::application::ports::net;
using arkan::thanatos::application::state::SessionRegistry;
using arkan::thanatos::infrastructure::log::Logger;

using TcpClient = arkan::thanatos::infrastructure::net::asio_impl::AsioTcpClient;

class RoBridgeHandler final : public ports::IConnectionHandler
{
   public:
    RoBridgeHandler(boost::asio::io_context& io, std::string host, std::uint16_t port)
        : io_(io), host_(std::move(host)), port_(port)
    {
    }

    RoBridgeHandler(boost::asio::io_context& io, std::string host, std::uint16_t port,
                    std::shared_ptr<SessionRegistry> registry)
        : io_(io), host_(std::move(host)), port_(port), registry_(std::move(registry))
    {
    }

    void on_connect(std::shared_ptr<ports::ISession> s) override;
    void on_data(std::shared_ptr<ports::ISession> s, std::span<const std::uint8_t> bytes) override;
    void on_disconnect(std::shared_ptr<ports::ISession> s, const std::error_code& ec) override;

   private:
    struct Peer
    {
        std::shared_ptr<ports::ISession> session;
        std::shared_ptr<TcpClient> ro;
    };

    boost::asio::io_context& io_;
    std::unordered_map<ports::ISession*, Peer> peers_;
    std::string host_;
    std::uint16_t port_;
    std::shared_ptr<SessionRegistry> registry_;
};

}  // namespace arkan::thanatos::interface::dev
