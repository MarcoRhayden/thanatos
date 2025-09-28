#pragma once

#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <span>
#include <string>
#include <system_error>
#include <vector>

#include "infrastructure/net/asio/AsioTypes.hpp"

namespace arkan::poseidon::infrastructure::net::asio_impl
{

class AsioTcpClient : public std::enable_shared_from_this<AsioTcpClient>
{
   public:
    using DataHandler = std::function<void(std::span<const std::uint8_t>)>;
    using ErrorHandler = std::function<void(const std::error_code&)>;

    explicit AsioTcpClient(asio::io_context& io)
        : io_(io), socket_(io), strand_(asio::make_strand(io))
    {
    }

    void connect(const std::string& host, std::uint16_t port);
    void send(std::span<const std::uint8_t> bytes);
    void close();

    void on_data(DataHandler h)
    {
        on_data_ = std::move(h);
    }
    void on_error(ErrorHandler h)
    {
        on_error_ = std::move(h);
    }

   private:
    static constexpr std::size_t kMaxWriteQueue = 1024;

    void do_resolve_and_connect(const std::string& host, std::uint16_t port);
    void do_read();
    void do_write();
    void fail(const boost::system::error_code& ec);

    asio::io_context& io_;
    tcp::socket socket_;
    asio::strand<asio::any_io_executor> strand_;
    std::vector<std::uint8_t> read_buf_{};
    std::deque<std::vector<std::uint8_t>> write_q_;

    DataHandler on_data_;
    ErrorHandler on_error_;
};

}  // namespace arkan::poseidon::infrastructure::net::asio_impl
