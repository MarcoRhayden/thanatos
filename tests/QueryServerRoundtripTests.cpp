#include <gtest/gtest.h>

#include <cstdint>
#include <functional>
#include <memory>
#include <span>
#include <vector>

#include "application/ports/net/ISession.hpp"
#include "application/state/SessionRegistry.hpp"
#include "interface/query/QueryHandler.hpp"
#include "interface/query/QueryProtocol.hpp"

namespace ports = arkan::poseidon::application::ports::net;
using arkan::poseidon::application::state::SessionRegistry;
using arkan::poseidon::interface::query::QueryHandler;
namespace wire = arkan::poseidon::interface::query::wire;

class FakeSession : public ports::ISession, public std::enable_shared_from_this<FakeSession>
{
   public:
    void send(std::span<const std::uint8_t> data) override
    {
        last_send.assign(data.begin(), data.end());
        if (on_send) on_send(last_send);
    }

    void close() override
    {
        closed = true;
    }

    std::string remote_endpoint() const override
    {
        return "fake:0";
    }

    std::function<void(const std::vector<std::uint8_t>&)> on_send;
    std::vector<std::uint8_t> last_send;
    bool closed{false};
};

class FakeClientSession : public ports::ISession,
                          public std::enable_shared_from_this<FakeClientSession>
{
   public:
    explicit FakeClientSession(SessionRegistry& reg) : reg_(reg) {}

    void send(std::span<const std::uint8_t> data) override
    {
        // When the QueryHandler "injects" the blob into the active client, we simulate
        // that this blob has become a C→S packet; we notify the registry.
        reg_.notify_c2s(data);
    }

    void close() override {}

    std::string remote_endpoint() const override
    {
        return "client:0";
    }

   private:
    SessionRegistry& reg_;
};

TEST(QueryRoundtrip, PoseidonQueryTriggersReplyUsingWaiter)
{
    auto registry = std::make_shared<SessionRegistry>();

    // QueryHandler (sem servidor real; chamamos APIs diretas)
    auto handler = std::make_shared<QueryHandler>(registry);

    auto querySess = std::make_shared<FakeSession>();  // lado "OpenKore"
    handler->on_connect(querySess);

    // registra cliente RO ativo
    auto roClient = std::make_shared<FakeClientSession>(*registry);
    registry->set_active_client(roClient);

    // “OpenKore” manda PoseidonQuery(blob)
    const std::uint8_t gg_blob[] = {0x10, 0x20, 0x30};
    auto payload = wire::encode_blob(std::span<const std::uint8_t>(gg_blob, sizeof(gg_blob)));
    auto frame = wire::frame(wire::MSG_POSEIDON_QUERY, payload);

    // Ao receber o Reply, validamos o framing e o payload
    bool got = false;
    querySess->on_send = [&](const std::vector<std::uint8_t>& out)
    {
        ASSERT_GE(out.size(), 4u);
        const auto size = wire::r16(out.data());
        const auto msg_id = wire::r16(out.data() + 2);
        EXPECT_EQ(size, out.size());
        EXPECT_EQ(msg_id, wire::MSG_POSEIDON_REPLY);

        std::vector<std::uint8_t> blob;
        const auto ok =
            wire::decode_blob(std::span<const std::uint8_t>(out.data() + 4, out.size() - 4), blob);
        ASSERT_TRUE(ok);
        ASSERT_EQ(blob.size(), sizeof(gg_blob));
        for (size_t i = 0; i < blob.size(); ++i) EXPECT_EQ(blob[i], gg_blob[i]);  // round-trip
        got = true;
    };

    // Alimenta o handler como se fosse a rede
    handler->on_data(querySess, std::span<const std::uint8_t>(frame.data(), frame.size()));

    // O FakeClientSession chama registry.notify_c2s(data) em send(), o que dispara o Reply
    EXPECT_TRUE(got);

    handler->on_disconnect(querySess, std::error_code{});
}
