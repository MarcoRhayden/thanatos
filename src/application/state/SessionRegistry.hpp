#pragma once

#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <span>

#include "application/ports/net/ISession.hpp"

namespace arkan::thanatos::application::state
{

class SessionRegistry
{
   public:
    using ISession = arkan::thanatos::application::ports::net::ISession;

    // Mark RO client session as "active" for GG
    void set_active_client(std::shared_ptr<ISession> s)
    {
        std::scoped_lock lk(m_);
        active_client_ = std::move(s);
    }

    std::shared_ptr<ISession> get_active_client()
    {
        std::scoped_lock lk(m_);
        return active_client_.lock();
    }

    // Raw traffic notifications (hooked on RoBridgeHandler)
    void notify_c2s(std::span<const std::uint8_t> bytes)
    {
        std::function<bool(std::span<const std::uint8_t>)> pred;
        std::function<void(std::span<const std::uint8_t>)> cb;
        {
            std::scoped_lock lk(m_);
            if (!pending_pred_ || !pending_cb_) return;
            pred = *pending_pred_;
            cb = *pending_cb_;
        }
        if (pred(bytes))
        {
            {
                std::scoped_lock lk(m_);
                pending_pred_.reset();
                pending_cb_.reset();
            }
            cb(bytes);
        }
    }

    void notify_s2c(std::span<const std::uint8_t>) {}  // reserved for future heuristics

    // Register a "waiter" for the next C->S packet that satisfies the predicate
    void wait_next_c2s(std::function<bool(std::span<const std::uint8_t>)> predicate,
                       std::function<void(std::span<const std::uint8_t>)> on_match)
    {
        std::scoped_lock lk(m_);
        pending_pred_ = std::move(predicate);
        pending_cb_ = std::move(on_match);
    }

   private:
    std::mutex m_;
    std::weak_ptr<ISession> active_client_;
    std::optional<std::function<bool(std::span<const std::uint8_t>)>> pending_pred_;
    std::optional<std::function<void(std::span<const std::uint8_t>)>> pending_cb_;
};

}  // namespace arkan::thanatos::application::state
