#pragma once

#include <deque>
#include <memory>
#include <span>
#include <unordered_map>
#include <vector>

#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "application/state/SessionRegistry.hpp"
#include "infrastructure/log/Logger.hpp"
#include "interface/query/QueryProtocol.hpp"

namespace arkan::poseidon::interface::query
{

namespace ports = arkan::poseidon::application::ports::net;
using arkan::poseidon::application::state::SessionRegistry;
using arkan::poseidon::infrastructure::log::Logger;

class QueryHandler final : public ports::IConnectionHandler
{
   public:
    explicit QueryHandler(std::shared_ptr<SessionRegistry> reg) : registry_(std::move(reg)) {}

    void on_connect(std::shared_ptr<ports::ISession> s) override;
    void on_data(std::shared_ptr<ports::ISession> s, std::span<const std::uint8_t> bytes) override;
    void on_disconnect(std::shared_ptr<ports::ISession> s, const std::error_code& ec) override;

   private:
    struct Conn
    {
        std::shared_ptr<ports::ISession> session;
        std::vector<std::uint8_t> buf;  // framing buffer [size][msg_id]
    };

    void handle_frame(Conn& c, std::span<const std::uint8_t> frame);

    std::unordered_map<ports::ISession*, Conn> peers_;
    std::shared_ptr<SessionRegistry> registry_;
};

}  // namespace arkan::poseidon::interface::query
