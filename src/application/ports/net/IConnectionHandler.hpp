#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <system_error>

namespace arkan::poseidon::application::ports::net
{

class ISession;

class IConnectionHandler
{
   public:
    virtual ~IConnectionHandler() = default;
    virtual void on_connect(std::shared_ptr<ISession> s) = 0;
    virtual void on_data(std::shared_ptr<ISession> s, std::span<const std::uint8_t> bytes) = 0;
    virtual void on_disconnect(std::shared_ptr<ISession> s, const std::error_code& ec) = 0;
};

}  // namespace arkan::poseidon::application::ports::net
