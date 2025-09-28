#pragma once

#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "application/state/SessionRegistry.hpp"
#include "infrastructure/config/Config.hpp"
#include "infrastructure/log/Logger.hpp"

namespace arkan::poseidon::interface::ro
{

namespace ports = arkan::poseidon::application::ports::net;
using arkan::poseidon::application::state::SessionRegistry;
using arkan::poseidon::infrastructure::log::Logger;
namespace apc = arkan::poseidon::infrastructure::config;

class LoginHandler final : public ports::IConnectionHandler
{
   public:
    explicit LoginHandler(std::shared_ptr<SessionRegistry> reg, apc::Config cfg)
        : registry_(std::move(reg)), cfg_(std::move(cfg))
    {
    }

    void on_connect(std::shared_ptr<ports::ISession> s) override
    {
        Logger::info("[login] connect: " + s->remote_endpoint());
        sessions_.emplace(s.get(), Conn{std::move(s)});
        if (registry_) registry_->set_active_client(sessions_.at(s.get()).session);
    }

    void on_disconnect(std::shared_ptr<ports::ISession> s, const std::error_code& ec) override
    {
        Logger::info("[login] disconnect: " + s->remote_endpoint() +
                     (ec ? (" ec=" + ec.message()) : ""));
        sessions_.erase(s.get());
    }

    void on_data(std::shared_ptr<ports::ISession> s, std::span<const std::uint8_t> bytes) override
    {
        (void)s;
        if (!bytes.empty())
        {
            Logger::debug("[login] recv " + std::to_string(bytes.size()) + " bytes");
        }
    }

   private:
    struct Conn
    {
        std::shared_ptr<ports::ISession> session;
    };

    std::unordered_map<ports::ISession*, Conn> sessions_;
    std::shared_ptr<SessionRegistry> registry_;
    apc::Config cfg_;
};

}  // namespace arkan::poseidon::interface::ro
