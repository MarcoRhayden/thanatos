#include "interface/query/QueryServer.hpp"

#include <array>
#include <boost/asio.hpp>
#include <cstring>
#include <string>

#include "infrastructure/log/Logger.hpp"

using arkan::poseidon::infrastructure::log::Logger;
using boost::asio::ip::tcp;

namespace arkan::poseidon::interface::query
{

struct QueryServer::Impl
{
    boost::asio::io_context& io;
    const std::string host;
    const std::uint16_t port;

    tcp::acceptor acc;
    std::unique_ptr<tcp::socket> sock;
    std::function<void(std::vector<std::uint8_t>)> on_query;

    Impl(boost::asio::io_context& io_, std::string host_, std::uint16_t port_)
        : io(io_), host(std::move(host_)), port(port_), acc(io)
    {
    }

    void start()
    {
        boost::system::error_code ec;
        auto addr = boost::asio::ip::make_address(host, ec);
        if (ec)
        {
            Logger::error(std::string("[query] invalid host: ") + host + " (" + ec.message() + ")");
            return;
        }

        tcp::endpoint ep(addr, port);
        acc.open(ep.protocol(), ec);
        if (ec)
        {
            Logger::error(std::string("[query] acceptor open failed: ") + ec.message());
            return;
        }

        acc.set_option(tcp::acceptor::reuse_address(true), ec);
        acc.bind(ep, ec);
        if (ec)
        {
            Logger::error(std::string("[query] bind failed at ") + host + ":" +
                          std::to_string(port) + " (" + ec.message() + ")");
            return;
        }

        acc.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec)
        {
            Logger::error(std::string("[query] listen failed: ") + ec.message());
            return;
        }

        Logger::info(std::string("[query] listening at ") + host + ":" + std::to_string(port));
        async_accept();
    }

    void stop()
    {
        Logger::info(std::string("[query] stopping"));
        boost::system::error_code ec;
        acc.cancel(ec);
        acc.close(ec);
        if (sock)
        {
            sock->cancel(ec);
            sock->close(ec);
        }
    }

    void async_accept()
    {
        sock = std::make_unique<tcp::socket>(io);
        acc.async_accept(
            *sock,
            [this](boost::system::error_code ec)
            {
                if (ec)
                {
                    Logger::debug(std::string("[query] accept error: ") + ec.message());
                    async_accept();
                    return;
                }

                std::string ep = "[unknown]";
                boost::system::error_code ep_ec;
                auto rem = sock->remote_endpoint(ep_ec);
                if (!ep_ec)
                {
                    ep = rem.address().to_string() + ":" + std::to_string(rem.port());
                }
                Logger::info(std::string("[query] client connected: ") + ep);

                read_frame_header();
            });
    }

    void read_frame_header()
    {
        auto buf = std::make_shared<std::array<std::uint8_t, 10>>();
        boost::asio::async_read(
            *sock, boost::asio::buffer(*buf),
            [this, buf](boost::system::error_code ec, std::size_t n)
            {
                if (ec || n != buf->size())
                {
                    if (ec)
                    {
                        Logger::debug(std::string("[query] read header error: ") + ec.message());
                    }
                    else
                    {
                        Logger::debug(std::string("[query] short header bytes: ") +
                                      std::to_string(n));
                    }
                    async_accept();
                    return;
                }

                if (std::memcmp(buf->data(), "PSDN", 4) != 0)
                {
                    Logger::debug(std::string("[query] invalid magic (not PSDN); dropping client"));
                    async_accept();
                    return;
                }

                std::uint16_t type = (std::uint16_t)buf->at(4) | ((std::uint16_t)buf->at(5) << 8);
                std::uint32_t len = (std::uint32_t)buf->at(6) | ((std::uint32_t)buf->at(7) << 8) |
                                    ((std::uint32_t)buf->at(8) << 16) |
                                    ((std::uint32_t)buf->at(9) << 24);

                auto data = std::make_shared<std::vector<std::uint8_t>>(len);
                boost::asio::async_read(
                    *sock, boost::asio::buffer(*data),
                    [this, type, data](boost::system::error_code ec2, std::size_t n2)
                    {
                        if (ec2 || n2 != data->size())
                        {
                            if (ec2)
                            {
                                Logger::debug(std::string("[query] read payload error: ") +
                                              ec2.message());
                            }
                            else
                            {
                                Logger::debug(std::string("[query] short payload bytes: ") +
                                              std::to_string(n2) +
                                              " expected=" + std::to_string(data->size()));
                            }
                            async_accept();
                            return;
                        }

                        const auto t_query =
                            (std::uint16_t)application::ports::query::MsgType::PoseidonQuery;
                        const auto t_reply =
                            (std::uint16_t)application::ports::query::MsgType::PoseidonReply;

                        if (type == t_query)
                        {
                            Logger::debug(std::string("[query] PoseidonQuery len=") +
                                          std::to_string(data->size()));
                            if (on_query) on_query(*data);
                        }
                        else if (type == t_reply)
                        {
                            Logger::debug(
                                std::string("[query] PoseidonReply (unexpected in server) len=") +
                                std::to_string(data->size()));
                        }
                        else
                        {
                            Logger::debug(std::string("[query] unknown type=") +
                                          std::to_string(type) +
                                          " len=" + std::to_string(data->size()));
                        }

                        // Continua lendo próximos frames do mesmo cliente.
                        read_frame_header();
                    });
            });
    }

    void send_reply(const std::vector<std::uint8_t>& payload)
    {
        if (!sock || !sock->is_open())
        {
            Logger::debug(std::string("[query] send_reply ignored (no active client socket)"));
            return;
        }

        std::vector<std::uint8_t> out;
        out.reserve(10 + payload.size());
        out.insert(out.end(), {'P', 'S', 'D', 'N'});

        const std::uint16_t type = (std::uint16_t)application::ports::query::MsgType::PoseidonReply;
        out.push_back((std::uint8_t)(type & 0xFF));
        out.push_back((std::uint8_t)((type >> 8) & 0xFF));

        const std::uint32_t len = (std::uint32_t)payload.size();
        out.push_back((std::uint8_t)(len & 0xFF));
        out.push_back((std::uint8_t)((len >> 8) & 0xFF));
        out.push_back((std::uint8_t)((len >> 16) & 0xFF));
        out.push_back((std::uint8_t)((len >> 24) & 0xFF));

        out.insert(out.end(), payload.begin(), payload.end());

        auto buf = std::make_shared<std::vector<std::uint8_t>>(std::move(out));
        Logger::debug(std::string("[query] send PoseidonReply len=") +
                      std::to_string(payload.size()));

        boost::asio::async_write(*sock, boost::asio::buffer(*buf),
                                 [buf](boost::system::error_code /*ec*/, std::size_t /*n*/)
                                 {
                                     // fire-and-forget
                                 });
    }
};

QueryServer::QueryServer(boost::asio::io_context& io, const std::string& host, std::uint16_t port)
    : impl_(std::make_unique<Impl>(io, host, port))
{
}

QueryServer::~QueryServer() = default;

void QueryServer::start()
{
    impl_->start();
}

void QueryServer::stop()
{
    impl_->stop();
}

void QueryServer::onQuery(std::function<void(std::vector<std::uint8_t>)> cb)
{
    impl_->on_query = std::move(cb);
}

void QueryServer::sendReply(const std::vector<std::uint8_t>& gg_reply)
{
    impl_->send_reply(gg_reply);
}

std::string QueryServer::endpoint_description() const
{
    return impl_->host + ":" + std::to_string(impl_->port);
}

}  // namespace arkan::poseidon::interface::query
