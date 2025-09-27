#pragma once
#include <memory>
#include <span>
#include <system_error>

#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "application/services/ILoginService.hpp"
#include "domain/protocol/Codec.hpp"
#include "domain/protocol/Parser.hpp"

namespace arkan::poseidon::interface::dev
{

class LoginHandlerDev : public application::ports::net::IConnectionHandler
{
   public:
    explicit LoginHandlerDev(std::unique_ptr<application::services::ILoginService> svc,
                             std::size_t maxpkt = 4 * 1024 * 1024)
        : svc_(std::move(svc)), parser_(maxpkt)
    {
    }

    void on_connect(std::shared_ptr<application::ports::net::ISession>) override {}
    void on_disconnect(std::shared_ptr<application::ports::net::ISession>,
                       const std::error_code&) override
    {
    }

    void on_data(std::shared_ptr<application::ports::net::ISession> s,
                 std::span<const std::uint8_t> bytes) override
    {
        parser_.feed(bytes);
        for (auto& p : parser_.drain())
        {
            auto replies = svc_->handle(p);
            for (auto& r : replies)
            {
                auto buf = domain::protocol::Encode(r);
                s->send(std::span<const std::uint8_t>(buf.data(), buf.size()));
            }
        }
    }

   private:
    std::unique_ptr<application::services::ILoginService> svc_;
    domain::protocol::Parser parser_;
};

}  // namespace arkan::poseidon::interface::dev
