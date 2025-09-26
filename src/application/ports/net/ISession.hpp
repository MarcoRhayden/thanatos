#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace arkan::poseidon::application::ports::net
{

class ISession
{
   public:
    virtual ~ISession() = default;
    virtual void send(std::span<const std::uint8_t> data) = 0;
    virtual void close() = 0;
    virtual std::string remote_endpoint() const = 0;
};

}  // namespace arkan::poseidon::application::ports::net
