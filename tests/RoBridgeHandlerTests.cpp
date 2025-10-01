#include <gtest/gtest.h>

#include <boost/asio.hpp>
#include <functional>
#include <future>
#include <span>
#include <string>
#include <thread>
#include <vector>

#include "application/ports/net/ISession.hpp"
#include "interface/dev/RoBridgeHandler.hpp"

namespace asio = boost::asio;
using tcp = asio::ip::tcp;

namespace ports = arkan::thanatos::application::ports::net;
using arkan::thanatos::interface::dev::RoBridgeHandler;

// FakeSession: captures send/close to validate the proxy
class FakeSession : public ports::ISession, public std::enable_shared_from_this<FakeSession>
{
   public:
    void send(std::span<const std::uint8_t> data) override
    {
        last_sent.assign(data.begin(), data.end());
        if (on_send) on_send(last_sent);
    }
    void close() override
    {
        closed = true;
        if (on_close) on_close();
    }
    std::string remote_endpoint() const override
    {
        return "fake:0";
    }

    std::function<void(const std::vector<uint8_t>&)> on_send;
    std::function<void()> on_close;
    std::vector<std::uint8_t> last_sent;
    bool closed{false};
};

TEST(RoBridge, ProxyEchoPath)
{
    asio::io_context io;

    // Upstream "RO real": echoes what it receives
    tcp::acceptor acc(io, tcp::endpoint(tcp::v4(), 0));
    const auto port = acc.local_endpoint().port();

    std::shared_ptr<tcp::socket> upstream = std::make_shared<tcp::socket>(io);
    acc.async_accept(
        *upstream,
        [&](const boost::system::error_code& ec)
        {
            ASSERT_FALSE(ec);

            auto buf = std::make_shared<std::vector<std::uint8_t>>(4096);
            auto do_read = std::make_shared<std::function<void()>>();
            *do_read = [&, buf, do_read]()
            {
                upstream->async_read_some(
                    asio::buffer(*buf),
                    [&, buf, do_read](const boost::system::error_code& bec, std::size_t n)
                    {
                        if (bec) return;
                        asio::async_write(
                            *upstream, asio::buffer(buf->data(), n),
                            [&, do_read](const boost::system::error_code& wec, std::size_t)
                            {
                                if (wec) return;
                                (*do_read)();
                            });
                    });
            };
            (*do_read)();
        });

    auto handler =
        std::make_shared<RoBridgeHandler>(io, "127.0.0.1", static_cast<std::uint16_t>(port));
    auto sess = std::make_shared<FakeSession>();

    std::promise<std::vector<uint8_t>> got;
    sess->on_send = [&](const std::vector<uint8_t>& bytes) { got.set_value(bytes); };

    handler->on_connect(sess);

    // Send via handler as if it were the client
    const std::string payload = "proxy-test";
    std::vector<std::uint8_t> bytes(payload.begin(), payload.end());
    handler->on_data(sess, std::span<const std::uint8_t>(bytes.data(), bytes.size()));

    std::thread th([&] { io.run(); });

    auto fut = got.get_future();
    ASSERT_EQ(fut.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    auto echoed = fut.get();

    ASSERT_EQ(echoed.size(), bytes.size());
    EXPECT_EQ(std::string(echoed.begin(), echoed.end()), payload);

    handler->on_disconnect(sess, std::error_code{});
    io.stop();
    th.join();
}
