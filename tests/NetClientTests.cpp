#include <gtest/gtest.h>

#include <atomic>
#include <boost/asio.hpp>
#include <future>
#include <string>
#include <thread>
#include <vector>

#include "infrastructure/net/asio/AsioTcpClient.hpp"

namespace asio = boost::asio;
using tcp = asio::ip::tcp;
using arkan::thanatos::infrastructure::net::asio_impl::AsioTcpClient;

TEST(NetClient, ConnectEchoAndClose)
{
    asio::io_context io;

    tcp::acceptor acc(io, tcp::endpoint(tcp::v4(), 0));
    const auto port = acc.local_endpoint().port();

    std::shared_ptr<tcp::socket> srv_sock = std::make_shared<tcp::socket>(io);
    acc.async_accept(
        *srv_sock,
        [&](const boost::system::error_code& ec)
        {
            ASSERT_FALSE(ec);
            auto buf = std::make_shared<std::vector<std::uint8_t>>(4096);
            auto do_read = std::make_shared<std::function<void()>>();
            *do_read = [&, buf, do_read]()
            {
                srv_sock->async_read_some(
                    asio::buffer(*buf),
                    [&, buf, do_read](const boost::system::error_code& bec, std::size_t n)
                    {
                        if (bec) return;
                        asio::async_write(
                            *srv_sock, asio::buffer(buf->data(), n),
                            [&, do_read](const boost::system::error_code& wec, std::size_t)
                            {
                                if (wec) return;
                                (*do_read)();
                            });
                    });
            };
            (*do_read)();
        });

    auto client = std::make_shared<AsioTcpClient>(io);

    std::promise<std::vector<std::uint8_t>> got;
    std::atomic<bool> got_set{false};

    client->on_data(
        [&](std::span<const std::uint8_t> bytes)
        {
            if (!got_set.exchange(true))
            {
                got.set_value(std::vector<std::uint8_t>(bytes.begin(), bytes.end()));
            }
        });

    std::promise<std::string> err_promise;
    std::atomic<bool> err_set{false};

    client->on_error(
        [&](const std::error_code& ec)
        {
            if (!err_set.exchange(true))
            {
                err_promise.set_value(ec.message());
            }
        });

    client->connect("127.0.0.1", static_cast<std::uint16_t>(port));

    std::thread th([&] { io.run(); });

    // send and wait for echo
    const std::string msg = "hello asio";
    client->send(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(msg.data()),
                                               msg.size()));

    auto fut = got.get_future();
    ASSERT_EQ(fut.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    auto echoed = fut.get();

    ASSERT_EQ(echoed.size(), msg.size());
    EXPECT_EQ(std::string(reinterpret_cast<char*>(echoed.data()), echoed.size()), msg);

    client->close();
    io.stop();
    th.join();
}

TEST(NetClient, UpstreamCloseTriggersError)
{
    asio::io_context io;
    tcp::acceptor acc(io, tcp::endpoint(tcp::v4(), 0));
    const auto port = acc.local_endpoint().port();

    acc.async_accept(
        [&](const boost::system::error_code& ec, tcp::socket socket)
        {
            ASSERT_FALSE(ec);
            boost::system::error_code x;
            socket.shutdown(boost::asio::socket_base::shutdown_both, x);
            socket.close(x);
        });

    auto client = std::make_shared<AsioTcpClient>(io);

    std::promise<std::string> err_promise;
    std::atomic<bool> err_set{false};

    client->on_error(
        [&](const std::error_code& ec)
        {
            if (!err_set.exchange(true))
            {
                err_promise.set_value(ec.message());
            }
        });

    client->connect("127.0.0.1", static_cast<std::uint16_t>(port));

    std::thread th([&] { io.run(); });

    auto fut = err_promise.get_future();
    ASSERT_EQ(fut.wait_for(std::chrono::seconds(2)), std::future_status::ready);
    auto msg = fut.get();
    EXPECT_FALSE(msg.empty());

    io.stop();
    th.join();
}
