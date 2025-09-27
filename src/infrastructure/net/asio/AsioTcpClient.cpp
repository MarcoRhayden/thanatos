#include "infrastructure/net/asio/AsioTcpClient.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <string>
#include <vector>

namespace arkan::poseidon::infrastructure::net::asio_impl
{

void AsioTcpClient::connect(const std::string& host, std::uint16_t port)
{
    auto self = shared_from_this();
    asio::dispatch(strand_, [this, self, host, port] { do_resolve_and_connect(host, port); });
}

void AsioTcpClient::do_resolve_and_connect(const std::string& host, std::uint16_t port)
{
    auto self = shared_from_this();

    boost::system::error_code ec;
    tcp::resolver resolver(io_);
    auto results = resolver.resolve(host, std::to_string(port), ec);
    if (ec)
    {
        fail(ec);
        return;
    }

    asio::async_connect(
        socket_, results,
        asio::bind_executor(strand_,
                            [this, self](const boost::system::error_code& bec, const tcp::endpoint&)
                            {
                                if (bec)
                                {
                                    fail(bec);
                                    return;
                                }
                                read_buf_.resize(4096);
                                do_read();
                            }));
}

void AsioTcpClient::send(std::span<const std::uint8_t> bytes)
{
    auto self = shared_from_this();
    std::vector<std::uint8_t> copy(bytes.begin(), bytes.end());

    asio::dispatch(strand_,
                   [this, self, buf = std::move(copy)]() mutable
                   {
                       write_q_.push_back(std::move(buf));
                       if (write_q_.size() == 1) do_write();
                   });
}

void AsioTcpClient::do_write()
{
    auto self = shared_from_this();
    asio::async_write(
        socket_, asio::buffer(write_q_.front()),
        asio::bind_executor(strand_,
                            [this, self](const boost::system::error_code& bec, std::size_t /*n*/)
                            {
                                if (bec)
                                {
                                    fail(bec);
                                    return;
                                }
                                write_q_.pop_front();
                                if (!write_q_.empty()) do_write();
                            }));
}

void AsioTcpClient::do_read()
{
    auto self = shared_from_this();
    socket_.async_read_some(
        asio::buffer(read_buf_),
        asio::bind_executor(strand_,
                            [this, self](const boost::system::error_code& bec, std::size_t n)
                            {
                                if (bec)
                                {
                                    fail(bec);
                                    return;
                                }
                                if (n > 0 && on_data_)
                                    on_data_(std::span<const std::uint8_t>(read_buf_.data(), n));
                                do_read();
                            }));
}

void AsioTcpClient::close()
{
    auto self = shared_from_this();
    asio::dispatch(strand_,
                   [this, self]
                   {
                       boost::system::error_code ec;
                       if (socket_.is_open())
                       {
                           socket_.shutdown(boost::asio::socket_base::shutdown_both, ec);
                           socket_.close(ec);
                       }
                       write_q_.clear();
                   });
}

void AsioTcpClient::fail(const boost::system::error_code& ec)
{
    // convert to std::error_code for the public callback (keeps API neutral)
    if (on_error_) on_error_(std::error_code(ec.value(), std::generic_category()));
}

}  // namespace arkan::poseidon::infrastructure::net::asio_impl
