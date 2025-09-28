#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "application/services/ILoginService.hpp"
#include "domain/protocol/Codec.hpp"
#include "domain/protocol/Parser.hpp"
#include "infrastructure/log/Logger.hpp"

namespace arkan::poseidon::interface::dev
{

namespace ports = arkan::poseidon::application::ports::net;
namespace svc = arkan::poseidon::application::services;
namespace proto = arkan::poseidon::domain::protocol;
using arkan::poseidon::infrastructure::log::Logger;

class LoginHandlerDev final : public ports::IConnectionHandler
{
   public:
    explicit LoginHandlerDev(std::shared_ptr<svc::ILoginService> service,
                             std::size_t parser_max = 0)
        : service_(std::move(service)), parser_max_(parser_max)
    {
    }

    void on_connect(std::shared_ptr<ports::ISession> s) override
    {
        Logger::info("[login] connect: " + s->remote_endpoint());

        auto& st = sessions_[s.get()];
        st.session = std::move(s);
        if (parser_max_ > 0)
        {
            st.parser.set_max(parser_max_);
        }
    }

    void on_data(std::shared_ptr<ports::ISession> s, std::span<const std::uint8_t> bytes) override
    {
        auto it = sessions_.find(s.get());
        if (it == sessions_.end()) return;

        auto& st = it->second;

        // Feed the parser with the received bytes
        st.parser.feed(bytes);

        // Drain all complete packets
        const auto packets = st.parser.drain();
        for (const auto& in_pkt : packets)
        {
            // The service can generate 0..N response packets
            auto outs = service_->handle(in_pkt);
            for (const auto& out_pkt : outs)
            {
                auto buf = proto::Encode(out_pkt);
                if (!buf.empty())
                {
                    st.session->send(buf);
                }
            }
        }
    }

    void on_disconnect(std::shared_ptr<ports::ISession> s, const std::error_code& ec) override
    {
        Logger::info("[login] disconnect: " + s->remote_endpoint() +
                     (ec ? (" ec=" + ec.message()) : ""));
        sessions_.erase(s.get());
    }

   private:
    struct ConnState
    {
        std::shared_ptr<ports::ISession> session;
        proto::Parser parser;
    };

    std::unordered_map<ports::ISession*, ConnState> sessions_;
    std::shared_ptr<svc::ILoginService> service_;
    std::size_t parser_max_ = 0;
};

}  // namespace arkan::poseidon::interface::dev
