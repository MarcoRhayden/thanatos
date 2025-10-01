#include "interface/query/QueryServer.hpp"

#include <boost/asio.hpp>
#include <memory>

#include "infrastructure/log/Logger.hpp"
#include "interface/query/bus/BusProtocol.hpp"

using arkan::thanatos::infrastructure::log::Logger;
using boost::asio::ip::tcp;
namespace bus = arkan::thanatos::interface::query::bus;

namespace arkan::thanatos::interface::query
{

struct QueryServer::Impl
{
    boost::asio::io_context& io;
    const std::string host;
    const uint16_t port;

    tcp::acceptor acc;
    std::unique_ptr<tcp::socket> sock;
    std::function<void(std::vector<uint8_t>)> on_query;

    std::array<uint8_t, 4> hdr{};  // length
    std::vector<uint8_t> frame;

    Impl(boost::asio::io_context& i, std::string h, uint16_t p)
        : io(i), host(std::move(h)), port(p), acc(io)
    {
    }

    void start()
    {
        boost::system::error_code ec;
        tcp::endpoint ep(boost::asio::ip::make_address(host, ec), port);
        if (ec)
        {
            Logger::error(std::string("[bus] bind address error: ") + ec.message());
            return;
        }
        acc.open(ep.protocol(), ec);
        acc.set_option(tcp::acceptor::reuse_address(true));
        acc.bind(ep, ec);
        if (ec)
        {
            Logger::error(std::string("[bus] bind failed: ") + ec.message());
            return;
        }
        acc.listen(boost::asio::socket_base::max_listen_connections, ec);
        if (ec)
        {
            Logger::error(std::string("[bus] listen failed: ") + ec.message());
            return;
        }
        Logger::info(std::string("[bus] listening on ") + host + ":" + std::to_string(port));
        async_accept();
    }

    void stop()
    {
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
        acc.async_accept(*sock,
                         [this](boost::system::error_code ec)
                         {
                             if (ec)
                             {
                                 async_accept();
                                 return;
                             }
                             Logger::info(std::string("[bus] client connected from ") +
                                          sock->remote_endpoint().address().to_string());
                             read_len();
                         });
    }

    void read_len()
    {
        auto self = sock.get();
        boost::asio::async_read(*self, boost::asio::buffer(hdr),
                                [this](boost::system::error_code ec, std::size_t n)
                                {
                                    if (ec || n != hdr.size())
                                    {
                                        recycle();
                                        return;
                                    }
                                    const uint32_t len = bus::rd_be32(hdr.data());
                                    if (len < 6 || len > (16u << 20))
                                    {  // sanity
                                        Logger::warn("[bus] invalid length frame dropped");
                                        recycle();
                                        return;
                                    }
                                    frame.resize(len);
                                    std::memcpy(frame.data(), hdr.data(), 4);
                                    read_rest(len - 4);
                                });
    }

    void read_rest(std::size_t remain)
    {
        auto self = sock.get();
        const std::size_t expected = remain;
        boost::asio::async_read(
            *self, boost::asio::buffer(frame.data() + 4, expected),
            [this, expected](boost::system::error_code ec, std::size_t n)
            {
                if (ec || n != expected)
                {
                    recycle();
                    return;
                }
                // parse
                try
                {
                    bus::Message msg;
                    bus::decode(frame.data(), frame.size(), msg);
                    std::string m =
                        "[bus] rx MID=" + msg.messageID + " options=" + std::to_string(msg.options);
                    Logger::debug(m);
                    // We expect ThanatosQuery: { packet = <bin> }
                    if (msg.options == 0 && msg.messageID == "ThanatosQuery")
                    {
                        auto it = msg.args_map.find("packet");
                        if (it != msg.args_map.end() && it->second.type == bus::ValueType::Binary)
                        {
                            if (on_query) on_query(it->second.bin);
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    Logger::warn(std::string("[bus] decode error: ") + e.what());
                }
                // loop same client
                read_len();
            });
    }

    void recycle()
    {
        // close and accept again
        boost::system::error_code ec;
        if (sock)
        {
            sock->shutdown(tcp::socket::shutdown_both, ec);
            sock->close(ec);
        }
        async_accept();
    }

    void send_reply_bin(const std::vector<uint8_t>& payload)
    {
        if (!sock || !sock->is_open())
        {
            Logger::debug("[bus] no client to reply");
            return;
        }
        bus::Message msg;
        msg.options = 0;  // map
        msg.messageID = "ThanatosReply";
        bus::Value v;
        v.type = bus::ValueType::Binary;
        v.bin = payload;
        msg.args_map.emplace("packet", std::move(v));
        auto buf = std::make_shared<std::vector<uint8_t>>(bus::encode(msg));
        boost::asio::async_write(*sock, boost::asio::buffer(*buf),
                                 [buf](boost::system::error_code, std::size_t) {});
    }
};

QueryServer::QueryServer(boost::asio::io_context& io, const std::string& host, uint16_t port)
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

void QueryServer::onQuery(std::function<void(std::vector<uint8_t>)> cb)
{
    impl_->on_query = std::move(cb);
}

void QueryServer::sendReply(const std::vector<uint8_t>& gg_reply)
{
    impl_->send_reply_bin(gg_reply);
}

std::string QueryServer::endpoint_description() const
{
    return impl_->host + ":" + std::to_string(impl_->port);
}

}  // namespace arkan::thanatos::interface::query
