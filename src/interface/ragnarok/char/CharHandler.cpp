#include "interface/ragnarok/char/CharHandler.hpp"

#include <cstring>

#include "interface/ragnarok/char/CharFlow.hpp"
#include "interface/ragnarok/proto/Codec.hpp"

namespace arkan
{
namespace poseidon
{
namespace interface
{
namespace ro
{

boost::system::error_code ec;
namespace ports_net = arkan::poseidon::application::ports::net;
using namespace arkan::poseidon::interface::ro::proto;

CharHandler::~CharHandler() = default;

CharHandler::CharHandler(std::shared_ptr<SessionRegistry> /*registry*/, const apc::Config& cfg,
                         OnLogFn logger)
    : log_(std::move(logger))
{
    auto addr = boost::asio::ip::make_address_v4(cfg.fakeIP, ec);
    if (!ec)
    {
        auto bytes = addr.to_bytes();
        cfg_.mapIp = {bytes[0], bytes[1], bytes[2], bytes[3]};
    }

    cfg_.mapPortLE = static_cast<uint16_t>(cfg.char_port);
    cfg_.charBlockSize = 155;
    cfg_.initialMap = "new_1-1.gat";
    cfg_.sex = 1;

    st_.accountID = {0x81, 0x84, 0x1E, 0x00};
    st_.selectedCharID = {0xE9, 0x03, 0x00, 0x00};

    flow_ = std::make_unique<charflow::CharFlow>(
        cfg_, st_,
        /*send*/ [this](const std::vector<uint8_t>& p) { sendBytes(p); },
        /*log */ [this](const std::string& m) { log(m); });
}

void CharHandler::log(const std::string& m)
{
    if (log_) log_(m);
}

void CharHandler::sendBytes(const std::vector<uint8_t>& p)
{
    if (auto s = cur_.lock()) s->send(std::span<const uint8_t>(p.data(), p.size()));
}

void CharHandler::on_connect(std::shared_ptr<ports_net::ISession> s)
{
    cur_ = s;
    log(std::string("connected: ") + s->remote_endpoint());

    if (g_expect_char_on_next_connect.exchange(false))
    {
        log("came from LoginFlow (ok)");
    }
}

void CharHandler::on_disconnect(std::shared_ptr<ports_net::ISession> s, const std::error_code&)
{
    (void)s;
    cur_.reset();
    log("disconnected");
}

void CharHandler::on_data(std::shared_ptr<ports_net::ISession> s,
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
