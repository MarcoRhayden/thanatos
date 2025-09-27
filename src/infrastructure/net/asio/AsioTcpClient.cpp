#include "AsioTcpClient.hpp"

#include <boost/system/error_code.hpp>

namespace arkan::poseidon::infrastructure::net::asio_impl
{

AsioTcpClient::AsioTcpClient(asio::io_context& io)
    : resolver_(io), socket_(io), strand_(socket_.get_executor())
{
}

void AsioTcpClient::connect(const std::string& host, std::uint16_t port, OnConnect cb)
{
    auto self = shared_from_this();
    resolver_.async_resolve(
        host, std::to_string(port),
        asio::bind_executor(
            strand_,
            [this, self, cb](const boost::system::error_code& ec,
                             tcp::resolver::results_type results)
            {
                if (ec)
                {
                    if (cb) cb(ec);
                    return;
                }
                asio::async_connect(
                    socket_, results,
                    asio::bind_executor(
                        strand_,
                        [this, self, cb](const boost::system::error_code& ec2, const tcp::endpoint&)
                        {
                            if (cb) cb(ec2);
                            if (!ec2) do_read();
                        }));
            }));
}

void AsioTcpClient::send(std::span<const std::uint8_t> bytes)
{
    auto self = shared_from_this();
    asio::dispatch(strand_,
                   [this, self, b = std::vector<std::uint8_t>(bytes.begin(), bytes.end())]() mutable
                   {
                       write_q_.push_back(std::move(b));
                       if (write_q_.size() == 1) do_write();
                   });
}

void AsioTcpClient::close()
{
    auto self = shared_from_this();
    asio::dispatch(strand_,
                   [this, self]
                   {
                       boost::system::error_code ec;
                       socket_.shutdown(asio::socket_base::shutdown_both, ec);
                       socket_.close(ec);
                   });
}

void AsioTcpClient::do_read()
{
    auto self = shared_from_this();
    socket_.async_read_some(
        asio::buffer(read_buf_),
        asio::bind_executor(strand_,
                            [this, self](const boost::system::error_code& ec, std::size_t n)
                            {
                                if (ec)
                                {
                                    if (on_disc_) on_disc_(ec);
                                    return;
                                }
                                if (on_data_)
                                    on_data_(std::span<const std::uint8_t>(read_buf_.data(), n));
                                do_read();
                            }));
}

void AsioTcpClient::do_write()
{
    auto self = shared_from_this();
    asio::async_write(
        socket_, asio::buffer(write_q_.front()),
        asio::bind_executor(strand_,
                            [this, self](const boost::system::error_code& ec, std::size_t)
                            {
                                if (ec)
                                {
                                    if (on_disc_) on_disc_(ec);
                                    return;
                                }
                                write_q_.pop_front();
                                if (!write_q_.empty()) do_write();
                            }));
}

}  // namespace arkan::poseidon::infrastructure::net::asio_impl
