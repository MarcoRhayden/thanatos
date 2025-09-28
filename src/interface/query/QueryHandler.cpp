#include "interface/query/QueryHandler.hpp"

using arkan::poseidon::interface::query::wire::decode_blob;
using arkan::poseidon::interface::query::wire::encode_blob;
using arkan::poseidon::interface::query::wire::frame;
using arkan::poseidon::interface::query::wire::MSG_POSEIDON_QUERY;
using arkan::poseidon::interface::query::wire::MSG_POSEIDON_REPLY;
using arkan::poseidon::interface::query::wire::r16;

namespace arkan::poseidon::interface::query
{

// Safety limit for the framing buffer (1 MB)
static constexpr std::size_t kMaxBuf = 1024 * 1024;

void QueryHandler::on_connect(std::shared_ptr<ports::ISession> s)
{
    Logger::info("[query] connect: " + s->remote_endpoint());
    peers_.emplace(s.get(), Conn{std::move(s), {}});
}

void QueryHandler::on_disconnect(std::shared_ptr<ports::ISession> s, const std::error_code& ec)
{
    Logger::info("[query] disconnect: " + s->remote_endpoint() +
                 (ec ? (" ec=" + ec.message()) : ""));
    peers_.erase(s.get());
}

void QueryHandler::on_data(std::shared_ptr<ports::ISession> s, std::span<const std::uint8_t> bytes)
{
    auto it = peers_.find(s.get());
    if (it == peers_.end()) return;
    auto& c = it->second;

    // framing: [u16 size][u16 msg_id][payload...], LE
    if (c.buf.size() + bytes.size() > kMaxBuf)
    {
        Logger::warn("[query] buffer overflow: clearing and closing");
        c.buf.clear();
        s->close();
        return;
    }

    c.buf.insert(c.buf.end(), bytes.begin(), bytes.end());

    for (;;)
    {
        if (c.buf.size() < 4) break;

        const auto size = r16(c.buf.data());
        if (size < 4)
        {
            // invalid size -> clear and exit (close on next read if persists)
            c.buf.clear();
            break;
        }

        if (c.buf.size() < size) break;

        std::vector<std::uint8_t> fr(c.buf.begin(), c.buf.begin() + size);
        c.buf.erase(c.buf.begin(), c.buf.begin() + size);

        handle_frame(c, std::span<const std::uint8_t>(fr.data(), fr.size()));
    }
}

void QueryHandler::handle_frame(Conn& c, std::span<const std::uint8_t> fr)
{
    if (fr.size() < 4) return;

    const auto size = r16(fr.data());
    const auto msg_id = r16(fr.data() + 2);
    const auto payload = fr.subspan(4, size - 4);

    if (msg_id == MSG_POSEIDON_QUERY)
    {
        // decode the blob
        std::vector<std::uint8_t> blob;
        if (!decode_blob(payload, blob)) return;

        // get the active RO client
        auto cli = registry_->get_active_client();
        if (!cli)
        {
            Logger::info("[query] no active client to deliver GG blob");
            return;
        }

        // "waiter": the next C->S packet becomes the Reply
        registry_->wait_next_c2s([](std::span<const std::uint8_t>) { return true; },
                                 [sess = c.session](std::span<const std::uint8_t> reply_bytes)
                                 {
                                     auto pld = encode_blob(reply_bytes);
                                     auto out = frame(MSG_POSEIDON_REPLY, pld);
                                     sess->send(out);
                                 });

        // inject the blob into the RO client (downstream)
        cli->send(std::span<const std::uint8_t>(blob.data(), blob.size()));
    }
    else
    {
        // unknown / ignored msg
    }
}

}  // namespace arkan::poseidon::interface::query
