#include "AsioTcpServer.hpp"

#include <array>
#include <boost/system/error_code.hpp>
#include <deque>
#include <span>

#include "application/ports/net/ISession.hpp"
#include "infrastructure/log/Logger.hpp"

using arkan::poseidon::infrastructure::log::Logger;
namespace ports = arkan::poseidon::application::ports::net;

namespace arkan::poseidon::infrastructure::net::asio_impl
{

class TcpSession final : public ports::ISession, public std::enable_shared_from_this<TcpSession>
{
   public:
    TcpSession(tcp::socket socket, std::shared_ptr<ports::IConnectionHandler> handler)
        : socket_(std::move(socket)), strand_(socket_.get_executor()), handler_(std::move(handler))
    {
    }

    void start()
    {
        auto self = shared_from_this();
        asio::dispatch(strand_, [this, self] { do_read(); });
    }

    void send(std::span<const std::uint8_t> data) override
    {
        auto self = shared_from_this();
        asio::dispatch(strand_,
                       [this, self, bytes = Buffer(data.begin(), data.end())]() mutable
                       {
                           write_queue_.push_back(std::move(bytes));
                           if (write_queue_.size() == 1) do_write();
                       });
    }

    void close() override
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

    std::string remote_endpoint() const override
    {
        boost::system::error_code ec;
        auto ep = socket_.remote_endpoint(ec);
        if (ec) return {};
        return ep.address().to_string() + ":" + std::to_string(ep.port());
    }

   private:
    void do_read()
    {
        auto self = shared_from_this();
        socket_.async_read_some(
            asio::buffer(read_buf_),
            asio::bind_executor(strand_,
                                [this, self](const boost::system::error_code& bec, std::size_t n)
                                {
                                    if (bec)
                                    {
                                        std::error_code ec = bec;
                                        handler_->on_disconnect(self, ec);
                                        return;
                                    }
                                    handler_->on_data(
                                        self, std::span<const std::uint8_t>(read_buf_.data(), n));
                                    do_read();
                                }));
    }

    void do_write()
    {
        auto self = shared_from_this();
        asio::async_write(
            socket_, asio::buffer(write_queue_.front()),
            asio::bind_executor(strand_,
                                [this, self](const boost::system::error_code& bec, std::size_t)
                                {
                                    if (bec)
                                    {
                                        std::error_code ec = bec;
                                        handler_->on_disconnect(self, ec);
                                        return;
                                    }
                                    write_queue_.pop_front();
                                    if (!write_queue_.empty()) do_write();
                                }));
    }

    tcp::socket socket_;
    asio::strand<tcp::socket::executor_type> strand_;
    std::array<std::uint8_t, 4096> read_buf_{};
    std::deque<Buffer> write_queue_;
    std::shared_ptr<ports::IConnectionHandler> handler_;
};

class AsioTcpServerImpl final : public ports::ITcpServer
{
   public:
    AsioTcpServerImpl(asio::io_context& io, std::uint16_t port,
                      std::shared_ptr<ports::IConnectionHandler> h)
        : io_(io),
          acceptor_(io, tcp::endpoint(tcp::v4(), port)),
          port_(port),
          handler_(std::move(h)),
          running_(false)
    {
    }

    void start() override
    {
        running_ = true;
        do_accept();
    }

    void stop() override
    {
        running_ = false;
        boost::system::error_code ec;
        acceptor_.cancel(ec);
        acceptor_.close(ec);
    }

    std::uint16_t port() const override
    {
        return port_;
    }
    bool is_running() const override
    {
        return running_;
    }

   private:
    void do_accept()
    {
        acceptor_.async_accept(
            [this](const boost::system::error_code& bec, tcp::socket socket)
            {
                if (!running_) return;
                if (!bec)
                {
                    auto session = std::make_shared<TcpSession>(std::move(socket), handler_);
                    handler_->on_connect(session);
                    session->start();
                }
                if (acceptor_.is_open()) do_accept();
            });
    }

    asio::io_context& io_;
    tcp::acceptor acceptor_;
    std::uint16_t port_;
    std::shared_ptr<ports::IConnectionHandler> handler_;
    bool running_;
};

std::unique_ptr<ports::ITcpServer> MakeTcpServer(asio::io_context& io, std::uint16_t port,
                                                 std::shared_ptr<ports::IConnectionHandler> handler)
{
    return std::unique_ptr<ports::ITcpServer>(new AsioTcpServerImpl(io, port, std::move(handler)));
}

}  // namespace arkan::poseidon::infrastructure::net::asio_impl
