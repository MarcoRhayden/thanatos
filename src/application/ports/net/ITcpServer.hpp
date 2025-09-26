#pragma once

#include <cstdint>
#include <memory>

namespace arkan::poseidon::application::ports::net
{

class IConnectionHandler;

class ITcpServer
{
   public:
    virtual ~ITcpServer() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual std::uint16_t port() const = 0;
    virtual bool is_running() const = 0;
};

}  // namespace arkan::poseidon::application::ports::net
