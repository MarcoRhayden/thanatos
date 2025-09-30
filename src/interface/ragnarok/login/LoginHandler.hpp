#pragma once

#include <array>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "LoginFlow.hpp"
#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "application/state/SessionRegistry.hpp"
#include "infrastructure/config/Config.hpp"

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

class LoginHandler : public ports_net::IConnectionHandler
{
   public:
    using OnLogFn = std::function<void(const std::string&)>;

    LoginHandler(std::shared_ptr<SessionRegistry> /*registry*/, const apc::Config& cfg,
                 OnLogFn logger = {});

    void on_connect(std::shared_ptr<ports_net::ISession> s) override;
    void on_disconnect(std::shared_ptr<ports_net::ISession> s, const std::error_code& ec) override;
    void on_data(std::shared_ptr<ports_net::ISession> s,
                 std::span<const std::uint8_t> bytes) override;

   private:
    void log(const std::string& msg);
    void sendBytes(const std::vector<uint8_t>& p);

   private:
    std::weak_ptr<ports_net::ISession> cur_;
    OnLogFn log_;

    loginflow::LoginCfg cfg_;
    loginflow::LoginState st_;
    std::unique_ptr<loginflow::LoginFlow> flow_;
};

}  // namespace ro
}  // namespace interface
}  // namespace poseidon
}  // namespace arkan
