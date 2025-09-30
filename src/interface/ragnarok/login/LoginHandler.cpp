#include "LoginHandler.hpp"

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
    cfg_.hostIp = {172, 65, 10, 90};
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
}

void LoginHandler::on_disconnect(std::shared_ptr<ports_net::ISession> s, const std::error_code& ec)
{
    (void)s;
    (void)ec;
    cur_.reset();
    log("disconnected");
}

void LoginHandler::on_data(std::shared_ptr<ports_net::ISession> s,
                           std::span<const std::uint8_t> bytes)
{
    cur_ = s;
    if (bytes.size() < 2) return;
    const uint16_t opcode = rd16le(bytes.data());
    flow_->handle(opcode, bytes.data() + 2, bytes.size() - 2);
}

}  // namespace ro
}  // namespace interface
}  // namespace poseidon
}  // namespace arkan
