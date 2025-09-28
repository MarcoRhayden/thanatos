#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "application/services/ICharService.hpp"
#include "application/state/SessionRegistry.hpp"
#include "domain/protocol/Codec.hpp"
#include "domain/protocol/Parser.hpp"
#include "infrastructure/log/Logger.hpp"

namespace arkan::poseidon::interface::dev
{

namespace ports = arkan::poseidon::application::ports::net;
namespace svc = arkan::poseidon::application::services;
namespace proto = arkan::poseidon::domain::protocol;
using arkan::poseidon::application::state::SessionRegistry;
using arkan::poseidon::infrastructure::log::Logger;

class CharHandlerDev final : public ports::IConnectionHandler
{
   public:
    explicit CharHandlerDev(std::shared_ptr<svc::ICharService> service,
                            std::shared_ptr<SessionRegistry> registry = {})
        : service_(std::move(service)), registry_(std::move(registry))
    {
    }

    void on_connect(std::shared_ptr<ports::ISession> s) override
    {
        Logger::info("[char] connect: " + s->remote_endpoint());

        if (registry_) registry_->set_active_client(s);

        const auto key = s.get();
        sessions_.emplace(key, ConnState{std::move(s), proto::Parser{}});
    }

    void on_data(std::shared_ptr<ports::ISession> s, std::span<const std::uint8_t> bytes) override
    {
        auto it = sessions_.find(s.get());
        if (it == sessions_.end()) return;

        auto& st = it->second;
        st.parser.feed(bytes);

        auto packets = st.parser.drain();
        for (auto& p : packets)
        {
            auto outs = service_->handle(p);
            for (auto& outp : outs)
            {
                auto buf = proto::Encode(outp);
                st.session->send(buf);
            }
        }
    }

    void on_disconnect(std::shared_ptr<ports::ISession> s, const std::error_code& ec) override
    {
        Logger::info("[char] disconnect: " + s->remote_endpoint() +
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
    std::shared_ptr<svc::ICharService> service_;
    std::shared_ptr<SessionRegistry> registry_;
};

}  // namespace arkan::poseidon::interface::dev
