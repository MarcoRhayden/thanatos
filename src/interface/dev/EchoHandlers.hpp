#pragma once

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <system_error>

#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "infrastructure/log/Logger.hpp"

namespace arkan::poseidon::interface::dev
{

class EchoHandler final : public application::ports::net::IConnectionHandler
{
   public:
    void on_connect(std::shared_ptr<application::ports::net::ISession> s) override
    {
        using arkan::poseidon::infrastructure::log::Logger;
        Logger::info(std::string("[Query] connect: ") + s->remote_endpoint());
        static constexpr char msg[] = "hello from QueryTcpServer\n";
        s->send(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(msg),
                                              sizeof(msg) - 1));
    }

    void on_data(std::shared_ptr<application::ports::net::ISession> s,
                 std::span<const std::uint8_t> bytes) override
    {
        s->send(bytes);  // echo
    }

    void on_disconnect(std::shared_ptr<application::ports::net::ISession> s,
                       const std::error_code&) override
    {
        using arkan::poseidon::infrastructure::log::Logger;
        Logger::info(std::string("[Query] disconnect: ") + s->remote_endpoint());
    }
};

class EchoHandlerRo final : public application::ports::net::IConnectionHandler
{
   public:
    void on_connect(std::shared_ptr<application::ports::net::ISession> s) override
    {
        using arkan::poseidon::infrastructure::log::Logger;
        Logger::info(std::string("[RO] connect: ") + s->remote_endpoint());
        static constexpr char msg[] = "hello from RoTcpServer\n";
        s->send(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(msg),
                                              sizeof(msg) - 1));
    }

    void on_data(std::shared_ptr<application::ports::net::ISession> s,
                 std::span<const std::uint8_t> bytes) override
    {
        s->send(bytes);  // echo
    }

    void on_disconnect(std::shared_ptr<application::ports::net::ISession> s,
                       const std::error_code&) override
    {
        using arkan::poseidon::infrastructure::log::Logger;
        Logger::info(std::string("[RO] disconnect: ") + s->remote_endpoint());
    }
};

}  // namespace arkan::poseidon::interface::dev
