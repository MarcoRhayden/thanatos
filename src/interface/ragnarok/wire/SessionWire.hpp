#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <vector>

#include "application/ports/net/IClientWire.hpp"
#include "application/ports/net/ISession.hpp"

namespace arkan
{
namespace poseidon
{
namespace interface
{
namespace ro
{
namespace wire
{

namespace ports_net = arkan::poseidon::application::ports::net;

// Thin adapter: writes raw bytes to the current RO session
class SessionWire final : public arkan::poseidon::application::ports::net::IClientWire
{
   public:
    explicit SessionWire(std::weak_ptr<ports_net::ISession> s) : s_(std::move(s)) {}

    void reset(std::weak_ptr<ports_net::ISession> s)
    {
        s_ = std::move(s);
    }

    bool send_to_client(const std::vector<std::uint8_t>& bytes) override
    {
        if (auto sp = s_.lock())
        {
            sp->send(std::span<const std::uint8_t>(bytes.data(), bytes.size()));
            return true;
        }
        return false;
    }

   private:
    std::weak_ptr<ports_net::ISession> s_;
};

}  // namespace wire
}  // namespace ro
}  // namespace interface
}  // namespace poseidon
}  // namespace arkan
