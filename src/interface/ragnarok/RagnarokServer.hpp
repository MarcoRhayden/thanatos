#pragma once

#include <memory>
#include <string>

#include "application/services/GameGuardBridge.hpp"
#include "application/state/SessionRegistry.hpp"
#include "infrastructure/config/Config.hpp"
#include "infrastructure/log/Logger.hpp"
#include "infrastructure/net/asio/AsioTcpServer.hpp"
#include "interface/query/QueryServer.hpp"
#include "interface/ragnarok/char/CharHandler.hpp"
#include "interface/ragnarok/login/LoginHandler.hpp"

namespace arkan::thanatos::interface::ro
{

namespace apc = arkan::thanatos::infrastructure::config;
namespace asio_impl = arkan::thanatos::infrastructure::net::asio_impl;
namespace ports_net = arkan::thanatos::application::ports::net;

using arkan::thanatos::application::state::SessionRegistry;
using arkan::thanatos::infrastructure::log::Logger;
using GB = arkan::thanatos::application::services::GameGuardBridge;

class RagnarokServer final
{
   public:
    RagnarokServer(boost::asio::io_context& io, std::shared_ptr<SessionRegistry> registry,
                   const apc::Config& cfg);
    void start();
    void stop();

   private:
    boost::asio::io_context& io_;
    std::shared_ptr<SessionRegistry> registry_;
    apc::Config cfg_;

    std::string roHost_;
    std::string qhost_;
    uint16_t qport_{};

    std::shared_ptr<LoginHandler> login_handler_;
    std::shared_ptr<CharHandler> char_handler_;

    std::unique_ptr<ports_net::ITcpServer> login_srv_;
    std::unique_ptr<ports_net::ITcpServer> char_srv_;
    std::unique_ptr<interface::query::QueryServer> query_server_;
    std::unique_ptr<application::services::GameGuardBridge> gg_bridge_;
};

}  // namespace arkan::thanatos::interface::ro
