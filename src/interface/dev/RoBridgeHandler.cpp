#include "interface/dev/RoBridgeHandler.hpp"

#include <span>

using arkan::poseidon::infrastructure::log::Logger;

namespace arkan::poseidon::interface::dev
{

void RoBridgeHandler::on_connect(std::shared_ptr<ports::ISession> s)
{
    Logger::info("[ro] connect: " + s->remote_endpoint());

    // Client to real RO server, using valid io_context
    auto ro = std::make_shared<TcpClient>(io_);

    // --- upstream callbacks (RO -> client)
    ro->on_data(
        [this, key = s.get()](std::span<const std::uint8_t> bytes)
        {
            auto it = peers_.find(key);
            if (it == peers_.end()) return;
            it->second.session->send(bytes);
        });

    ro->on_error(
        [this, key = s.get()](const std::error_code& ec)
        {
            auto it = peers_.find(key);
            if (it == peers_.end()) return;
            Logger::info(std::string("[ro] upstream error: ") + ec.message());
            it->second.session->close();
        });

    Peer p;
    p.session = s;
    p.ro = ro;
    peers_.emplace(s.get(), std::move(p));

    try
    {
        ro->connect(host_, port_);
    }
    catch (const std::exception& e)
    {
        Logger::info(std::string("[ro] connect upstream failed: ") + e.what());
        s->close();
        peers_.erase(s.get());
    }
}

void RoBridgeHandler::on_data(std::shared_ptr<ports::ISession> s,
                              std::span<const std::uint8_t> bytes)
{
    auto it = peers_.find(s.get());
    if (it == peers_.end()) return;

    // cliente -> RO
    it->second.ro->send(bytes);
}

void RoBridgeHandler::on_disconnect(std::shared_ptr<ports::ISession> s, const std::error_code& ec)
{
    auto it = peers_.find(s.get());
    if (it == peers_.end()) return;

    Logger::info("[ro] disconnect: " + s->remote_endpoint() + (ec ? (" ec=" + ec.message()) : ""));

    it->second.ro->close();
    peers_.erase(it);
}

}  // namespace arkan::poseidon::interface::dev
