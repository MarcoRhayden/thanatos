#pragma once

#include <cstdint>

namespace arkan::poseidon::application::ports::net
{

class ITcpServer
{
   public:
    virtual ~ITcpServer() = 0;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual std::uint16_t port() const = 0;
    virtual bool is_running() const = 0;
};

inline ITcpServer::~ITcpServer() = default;

}  // namespace arkan::poseidon::application::ports::net
