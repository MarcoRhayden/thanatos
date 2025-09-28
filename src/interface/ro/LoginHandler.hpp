#pragma once

#include <memory>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "application/state/SessionRegistry.hpp"
#include "infrastructure/config/Config.hpp"
#include "infrastructure/log/Logger.hpp"
#include "shared/Hex.hpp"

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
        // === configure reply blobs (hex) ===
        constexpr const char* kLoginOkHex = "";     // e.g.: "69 00 01 00 00 00 ..."
        constexpr const char* kServerListHex = "";  // e.g.: "6A 00 01 00 ... ip/port ..."

        if (!shared::hex::parse_hex_string(kLoginOkHex, login_ok_))
        {
            if (std::string(kLoginOkHex).empty())
                Logger::warn("[login] Phase 2: login_ok hex not set; no reply will be sent");
            else
                Logger::warn("[login] Phase 2: invalid login_ok hex; parsing failed");
        }

        if (!shared::hex::parse_hex_string(kServerListHex, server_list_))
        {
            if (std::string(kServerListHex).empty())
                Logger::warn("[login] Phase 2: server_list hex not set; no reply will be sent");
            else
                Logger::warn("[login] Phase 2: invalid server_list hex; parsing failed");
        }
    }

    void on_connect(std::shared_ptr<ports::ISession> s) override
    {
        Logger::info("[login] connect: " + s->remote_endpoint());
        sessions_.emplace(s.get(), Conn{std::move(s), /*sent*/ false});
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
        auto it = sessions_.find(s.get());
        if (it == sessions_.end()) return;
        auto& st = it->second;

        // first packet from client triggers minimal replies
        if (!bytes.empty())
            Logger::debug("[login] recv " + std::to_string(bytes.size()) + " bytes");

        if (!st.sent_replies)
        {
            // Send login_ok then server_list
            if (!login_ok_.empty())
            {
                st.session->send(std::span<const std::uint8_t>(login_ok_.data(), login_ok_.size()));
                Logger::debug("[login] sent login_ok (" + std::to_string(login_ok_.size()) +
                              " bytes)");
            }
            else
            {
                Logger::warn("[login] login_ok blob not set — skipping reply");
            }

            if (!server_list_.empty())
            {
                st.session->send(
                    std::span<const std::uint8_t>(server_list_.data(), server_list_.size()));
                Logger::debug("[login] sent server_list (" + std::to_string(server_list_.size()) +
                              " bytes)");
            }
            else
            {
                Logger::warn("[login] server_list blob not set — skipping reply");
            }

            st.sent_replies = true;
        }
    }

   private:
    struct Conn
    {
        std::shared_ptr<ports::ISession> session;
        bool sent_replies = false;
    };

    std::unordered_map<ports::ISession*, Conn> sessions_;
    std::shared_ptr<SessionRegistry> registry_;
    apc::Config cfg_;

    std::vector<std::uint8_t> login_ok_;
    std::vector<std::uint8_t> server_list_;
};

}  // namespace arkan::poseidon::interface::ro
