#pragma once
#include <memory>
#include <span>
#include <string>
#include <system_error>

#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "infrastructure/log/Logger.hpp"
#include "infrastructure/net/asio/AsioTcpClient.hpp"

namespace arkan::poseidon::interface::dev
{

class RoBridgeHandler : public application::ports::net::IConnectionHandler
{
   public:
    RoBridgeHandler(boost::asio::io_context& io, std::string host, std::uint16_t port)
        : io_(io), host_(std::move(host)), port_(port)
    {
    }

    void on_connect(std::shared_ptr<application::ports::net::ISession> down) override
    {
        using arkan::poseidon::infrastructure::log::Logger;
        client_ =
            std::make_shared<arkan::poseidon::infrastructure::net::asio_impl::AsioTcpClient>(io_);
        client_->set_on_data([down](std::span<const std::uint8_t> b) { down->send(b); });
        client_->set_on_disconnect([down](const boost::system::error_code&) { down->close(); });
        client_->connect(host_, port_,
                         [down](const boost::system::error_code& ec)
                         {
                             if (ec) down->close();
                         });
        Logger::info("RO bridge: downstream connected; dialing OpenKore...");
    }

    void on_data(std::shared_ptr<application::ports::net::ISession>,
                 std::span<const std::uint8_t> b) override
    {
        if (client_) client_->send(b);
    }

    void on_disconnect(std::shared_ptr<application::ports::net::ISession>,
                       const std::error_code&) override
    {
        if (client_) client_->close();
    }

   private:
    boost::asio::io_context& io_;
    std::string host_;
    std::uint16_t port_;
    std::shared_ptr<arkan::poseidon::infrastructure::net::asio_impl::AsioTcpClient> client_;
};

}  // namespace arkan::poseidon::interface::dev
