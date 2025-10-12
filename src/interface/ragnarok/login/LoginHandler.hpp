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
#include "application/ports/crypto/IRng.hpp"
#include "application/ports/crypto/ITokenGenerator.hpp"
#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "application/services/GameGuardBridge.hpp"
#include "application/state/SessionRegistry.hpp"
#include "infrastructure/config/Config.hpp"
#include "infrastructure/crypto/Random16TokenGenerator.hpp"
#include "interface/ragnarok/wire/SessionWire.hpp"
#include "shared/Hex.hpp"

#if defined(_WIN32) && defined(THANATOS_USE_WINCNG)
#include "infrastructure/crypto/WinCngRng.hpp"
#else
#include <random>

namespace arkan::thanatos::infrastructure::crypto
{
class PortableRng final : public arkan::thanatos::application::ports::crypto::IRng
{
   public:
    bool random(std::span<std::uint8_t> out) override
    {
        static thread_local std::random_device rd;
        for (auto& b : out) b = static_cast<std::uint8_t>(rd());
        return true;
    }
};
}  // namespace arkan::thanatos::infrastructure::crypto
#endif

namespace arkan
{
namespace thanatos
{
namespace interface
{
namespace ro
{

namespace ports_net = arkan::thanatos::application::ports::net;
using arkan::thanatos::application::state::SessionRegistry;
namespace apc = arkan::thanatos::infrastructure::config;
namespace hex = arkan::thanatos::shared::hex;

class LoginHandler : public ports_net::IConnectionHandler
{
   public:
    using OnLogFn = std::function<void(const std::string&)>;

    LoginHandler(std::shared_ptr<SessionRegistry> /*registry*/, const apc::Config& cfg,
                 OnLogFn logger = {});

    // Injects the GameGuard orchestrator
    void set_gg_bridge(arkan::thanatos::application::services::GameGuardBridge* gg)
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

    std::unique_ptr<arkan::thanatos::application::ports::crypto::IRng> rng_;
    std::unique_ptr<arkan::thanatos::application::ports::crypto::ITokenGenerator> tokenGen_;

    // bridge to GG + session adapter
    arkan::thanatos::application::services::GameGuardBridge* gg_{nullptr};
    std::unique_ptr<wire::SessionWire> wire_;
};

}  // namespace ro
}  // namespace interface
}  // namespace thanatos
}  // namespace arkan
