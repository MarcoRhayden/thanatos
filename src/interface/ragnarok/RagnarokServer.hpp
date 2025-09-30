#pragma once

#include <memory>
#include <string>

#include "application/state/SessionRegistry.hpp"
#include "infrastructure/config/Config.hpp"
#include "infrastructure/log/Logger.hpp"
#include "infrastructure/net/asio/AsioTcpServer.hpp"
#include "interface/ragnarok/char/CharHandler.hpp"
#include "interface/ragnarok/login/LoginHandler.hpp"

namespace arkan::poseidon::interface::ro
{

namespace apc = arkan::poseidon::infrastructure::config;
namespace asio_impl = arkan::poseidon::infrastructure::net::asio_impl;
namespace ports_net = arkan::poseidon::application::ports::net;

using arkan::poseidon::application::state::SessionRegistry;
using arkan::poseidon::infrastructure::log::Logger;

class RagnarokServer final
{
   public:
    RagnarokServer(boost::asio::io_context& io, std::shared_ptr<SessionRegistry> registry,
                   const apc::Config& cfg);

    void start();
    void iterate();
    void stop();

   private:
    std::shared_ptr<SessionRegistry> registry_;
    apc::Config cfg_;

    std::shared_ptr<LoginHandler> login_handler_;
    std::shared_ptr<CharHandler> char_handler_;

    std::unique_ptr<ports_net::ITcpServer> login_srv_;
    std::unique_ptr<ports_net::ITcpServer> char_srv_;
};

}  // namespace arkan::poseidon::interface::ro
