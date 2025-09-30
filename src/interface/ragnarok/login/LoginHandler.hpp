#pragma once

#include <array>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/system/error_code.hpp>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "LoginFlow.hpp"
#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "application/services/GameGuardBridge.hpp"
#include "application/state/SessionRegistry.hpp"
#include "infrastructure/config/Config.hpp"
#include "interface/ragnarok/wire/SessionWire.hpp"
#include "shared/Hex.hpp"

namespace arkan
{
namespace poseidon
{
namespace interface
{
namespace ro
{

namespace ports_net = arkan::poseidon::application::ports::net;
using arkan::poseidon::application::state::SessionRegistry;
namespace apc = arkan::poseidon::infrastructure::config;
namespace hex = arkan::poseidon::shared::hex;

class LoginHandler : public ports_net::IConnectionHandler
{
   public:
    using OnLogFn = std::function<void(const std::string&)>;

    LoginHandler(std::shared_ptr<SessionRegistry> /*registry*/, const apc::Config& cfg,
                 OnLogFn logger = {});

    // Injects the GameGuard orchestrator
    void set_gg_bridge(arkan::poseidon::application::services::GameGuardBridge* gg)
    {
        gg_ = gg;
    }

    void on_connect(std::shared_ptr<ports_net::ISession> s) override;
    void on_disconnect(std::shared_ptr<ports_net::ISession> s, const std::error_code& ec) override;
    void on_data(std::shared_ptr<ports_net::ISession> s,
                 std::span<const std::uint8_t> bytes) override;

   private:
    void log(const std::string& msg);
    void sendBytes(const std::vector<uint8_t>& p);

    std::weak_ptr<ports_net::ISession> cur_;
    OnLogFn log_;

    loginflow::LoginCfg cfg_;
    loginflow::LoginState st_;
    std::unique_ptr<loginflow::LoginFlow> flow_;

    // bridge to GG + session adapter
    arkan::poseidon::application::services::GameGuardBridge* gg_{nullptr};
    std::unique_ptr<wire::SessionWire> wire_;
};

}  // namespace ro
}  // namespace interface
}  // namespace poseidon
}  // namespace arkan
