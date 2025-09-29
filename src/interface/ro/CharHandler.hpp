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

class CharHandler final : public ports::IConnectionHandler
{
   public:
    explicit CharHandler(std::shared_ptr<SessionRegistry> reg, apc::Config cfg)
        : registry_(std::move(reg)), cfg_(std::move(cfg))
    {
        // === Phase 3: configure reply blobs (hex) ===
        constexpr const char* kCharListHex = "";  // e.g.: "6B 00 ...."  (char list)
        constexpr const char* kCharSelectAck = "";

        if (!shared::hex::parse_hex_string(kCharListHex, char_list_))
        {
            if (std::string(kCharListHex).empty())
                Logger::warn("[char] Phase 3: char_list hex not set; no list will be sent");
            else
                Logger::warn("[char] Phase 3: invalid char_list hex; parsing failed");
        }
        if (!shared::hex::parse_hex_string(kCharSelectAck, char_select_ack_))
        {
            if (std::string(kCharSelectAck).empty())
                Logger::info("[char] Phase 3: select ack hex not set (optional)");
            else
                Logger::warn("[char] Phase 3: invalid select ack hex; parsing failed");
        }
    }

    void on_connect(std::shared_ptr<ports::ISession> s) override
    {
        Logger::info("[char] connect: " + s->remote_endpoint());
        sessions_.emplace(s.get(), Conn{std::move(s)});
        if (registry_) registry_->set_active_client(sessions_.at(s.get()).session);
    }

    void on_disconnect(std::shared_ptr<ports::ISession> s, const std::error_code& ec) override
    {
        Logger::info("[char] disconnect: " + s->remote_endpoint() +
                     (ec ? (" ec=" + ec.message()) : ""));
        sessions_.erase(s.get());
    }

    void on_data(std::shared_ptr<ports::ISession> s, std::span<const std::uint8_t> bytes) override
    {
        auto it = sessions_.find(s.get());
        if (it == sessions_.end()) return;
        auto& st = it->second;

        if (!bytes.empty()) Logger::debug("[char] recv " + std::to_string(bytes.size()) + " bytes");

        if (!st.sent_char_list)
        {
            if (!char_list_.empty())
            {
                st.session->send(
                    std::span<const std::uint8_t>(char_list_.data(), char_list_.size()));
                Logger::debug("[char] sent char_list (" + std::to_string(char_list_.size()) +
                              " bytes)");
                st.sent_char_list = true;
            }
            else
            {
                Logger::warn("[char] char_list blob not set — skipping");
            }
            return;
        }

        if (!st.sent_select_ack && !char_select_ack_.empty())
        {
            st.session->send(
                std::span<const std::uint8_t>(char_select_ack_.data(), char_select_ack_.size()));
            Logger::debug("[char] sent select_ack (" + std::to_string(char_select_ack_.size()) +
                          " bytes)");
            st.sent_select_ack = true;
        }
    }

   private:
    struct Conn
    {
        std::shared_ptr<ports::ISession> session;
        bool sent_char_list = false;
        bool sent_select_ack = false;
    };

    std::unordered_map<ports::ISession*, Conn> sessions_;
    std::shared_ptr<SessionRegistry> registry_;
    apc::Config cfg_;

    std::vector<std::uint8_t> char_list_;
    std::vector<std::uint8_t> char_select_ack_;
};

}  // namespace arkan::poseidon::interface::ro
