#include "interface/ragnarok/login/LoginHandler.hpp"

#include "interface/ragnarok/proto/Codec.hpp"

namespace arkan
{
namespace poseidon
{
namespace interface
{
namespace ro
{

namespace ports_net = arkan::poseidon::application::ports::net;
using namespace arkan::poseidon::interface::ro::proto;

LoginHandler::LoginHandler(std::shared_ptr<SessionRegistry> /*registry*/, const apc::Config& cfg,
                           OnLogFn logger)
    : log_(std::move(logger))
{
    boost::system::error_code ec;
    auto addr = boost::asio::ip::make_address_v4(cfg.fakeIP, ec);
    if (!ec)
    {
        auto bytes = addr.to_bytes();
        cfg_.hostIp = {bytes[0], bytes[1], bytes[2], bytes[3]};
    }

    cfg_.hostPortLE = static_cast<uint16_t>(cfg.char_port);
    cfg_.serverName = "Arkan Software";
    cfg_.usersOnline = 77;
    cfg_.male = true;
    cfg_.prefer0069 = true;

    flow_ = std::make_unique<loginflow::LoginFlow>(
        cfg_, st_,
        /*send*/ [this](const std::vector<uint8_t>& p) { sendBytes(p); },
        /*log */ [this](const std::string& m) { log(m); });
}

void LoginHandler::log(const std::string& msg)
{
    if (log_) log_(msg);
}

void LoginHandler::sendBytes(const std::vector<uint8_t>& p)
{
    if (auto s = cur_.lock()) s->send(std::span<const uint8_t>(p.data(), p.size()));
}

void LoginHandler::on_connect(std::shared_ptr<ports_net::ISession> s)
{
    cur_ = s;
    log("connected: " + s->remote_endpoint());

    // SessionWire bind to bridge
    if (gg_)
    {
        if (!wire_)
            wire_ = std::make_unique<wire::SessionWire>(cur_);
        else
            wire_->reset(cur_);
        gg_->bindClientWire(wire_.get());
    }
}

void LoginHandler::on_disconnect(std::shared_ptr<ports_net::ISession> s, const std::error_code& ec)
{
    (void)s;
    (void)ec;
    cur_.reset();

    if (gg_) gg_->bindClientWire(nullptr);

    log("disconnected");
}

void LoginHandler::on_data(std::shared_ptr<ports_net::ISession> s,
                           std::span<const std::uint8_t> bytes)
{
    cur_ = s;

    if (gg_) gg_->onClientPacket(bytes.data(), bytes.size());

    if (bytes.size() < 2) return;
    const uint16_t op = rd16le(bytes.data());
    log(std::string("[login] rx opcode=0x") + arkan::poseidon::shared::hex::hex16(op) +
        " len=" + std::to_string(bytes.size() - 2));

    flow_->handle(op, bytes.data() + 2, bytes.size() - 2);
}

}  // namespace ro
}  // namespace interface
}  // namespace poseidon
}  // namespace arkan
