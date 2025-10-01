#pragma once

#include <span>
#include <unordered_map>
#include <vector>

#include "application/ports/net/IConnectionHandler.hpp"
#include "application/ports/net/ISession.hpp"
#include "application/state/SessionRegistry.hpp"
#include "infrastructure/log/Logger.hpp"

namespace arkan::thanatos::interface::query
{

namespace ports = arkan::thanatos::application::ports::net;
using arkan::thanatos::application::state::SessionRegistry;
using arkan::thanatos::infrastructure::log::Logger;

class QueryHandler final : public ports::IConnectionHandler
{
   public:
    explicit QueryHandler(std::shared_ptr<SessionRegistry> reg, std::size_t max_buf)
        : registry_(std::move(reg)), max_buf_(max_buf)
    {
    }

    void on_connect(std::shared_ptr<ports::ISession> s) override;
    void on_disconnect(std::shared_ptr<ports::ISession> s, const std::error_code& ec) override;
    void on_data(std::shared_ptr<ports::ISession> s, std::span<const std::uint8_t> bytes) override;

   private:
    struct Conn
    {
        std::shared_ptr<ports::ISession> session;
        std::vector<std::uint8_t> buf;
    };

    void handle_frame(Conn& c, std::span<const std::uint8_t> fr);

    std::unordered_map<ports::ISession*, Conn> peers_;
    std::shared_ptr<SessionRegistry> registry_;
    std::size_t max_buf_ = 1024 * 1024;  // default fallback
};

}  // namespace arkan::thanatos::interface::query
