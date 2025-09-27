#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "AsioTypes.hpp"

namespace arkan::poseidon::infrastructure::net::asio_impl
{

class AsioTcpClient : public std::enable_shared_from_this<AsioTcpClient>
{
   public:
    using OnConnect = std::function<void(const boost::system::error_code&)>;
    using OnData = std::function<void(std::span<const std::uint8_t>)>;
    using OnDisconnect = std::function<void(const boost::system::error_code&)>;

    explicit AsioTcpClient(asio::io_context& io);

    void connect(const std::string& host, std::uint16_t port, OnConnect cb);
    void send(std::span<const std::uint8_t> bytes);
    void close();

    void set_on_data(OnData cb)
    {
        on_data_ = std::move(cb);
    }
    void set_on_disconnect(OnDisconnect cb)
    {
        on_disc_ = std::move(cb);
    }

   private:
    void do_read();
    void do_write();

    tcp::resolver resolver_;
    tcp::socket socket_;
    asio::strand<tcp::socket::executor_type> strand_;
    std::array<std::uint8_t, 4096> read_buf_{};
    std::deque<std::vector<std::uint8_t>> write_q_;

    OnData on_data_;
    OnDisconnect on_disc_;
};

}  // namespace arkan::poseidon::infrastructure::net::asio_impl
