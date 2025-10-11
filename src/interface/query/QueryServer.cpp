#include "interface/query/QueryServer.hpp"

#include <boost/asio.hpp>
#include <memory>

#include "infrastructure/log/Logger.hpp"
#include "interface/query/bus/BusProtocol.hpp"
#include "shared/LogFmt.hpp"

using arkan::thanatos::infrastructure::log::Logger;
using arkan::thanatos::shared::logfmt::banner;
using arkan::thanatos::shared::logfmt::hex_dump;
using arkan::thanatos::shared::logfmt::ro_header;
using boost::asio::ip::tcp;
namespace bus = arkan::thanatos::interface::query::bus;

namespace arkan::thanatos::interface::query
{

// Thin Pimpl to keep header stable and isolate Boost.Asio details.
// ヘッダの安定性を保ちつつ Boost.Asio 実装を隠蔽する薄い Pimpl。
struct QueryServer::Impl
{
    boost::asio::io_context& io;
    const std::string host;
    const uint16_t port;

    tcp::acceptor acc;
    std::unique_ptr<tcp::socket> sock;
    std::function<void(std::vector<uint8_t>)> on_query;

    std::array<uint8_t, 4> hdr{};  // first 4 bytes = big-endian frame length
                                   // 先頭4バイトはビッグエンディアン長
    std::vector<uint8_t> frame;    // entire frame buffer (length + payload)
                                   // フレーム全体（長＋本体）

    Impl(boost::asio::io_context& i, std::string h, uint16_t p)
        : io(i), host(std::move(h)), port(p), acc(io)
    {
    }

    // Bring up the TCP acceptor and start the async accept loop.
    // TCP アクセプタを起動し、非同期 accept ループを開始する。
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

    // Stop accepting and tear down current socket if any.
    // 受け付けを停止し、接続済みソケットがあれば閉じる。
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

    // Accept a single client and immediately continue accepting the next one.
    // クライアントを1件受け付け、直ちに次の accept に進む。
    void async_accept()
    {
        sock = std::make_unique<tcp::socket>(io);
        acc.async_accept(*sock,
                         [this](boost::system::error_code ec)
                         {
                             if (ec)
                             {
                                 // Keep the server alive; keep accepting.
                                 // サーバを生かし続けるため、常に再度 accept。
                                 async_accept();
                                 return;
                             }
                             Logger::info(std::string("[bus] client connected from ") +
                                          sock->remote_endpoint().address().to_string());
                             read_len();
                         });
    }

    // Read the 4-byte big-endian length prefix.
    // 4 バイトのビッグエンディアン長プリフィクスを読む。
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
                                    // Basic sanity: minimal frame size and an upper bound to avoid
                                    // memory abuse. 最低限の長さと上限を設定（メモリ悪用対策）。
                                    if (len < 6 || len > (16u << 20))
                                    {
                                        Logger::warn("[bus] invalid length frame dropped");
                                        recycle();
                                        return;
                                    }
                                    frame.resize(len);
                                    // Preserve the length header at the front of the frame buffer.
                                    // フレーム先頭に長ヘッダを保持。
                                    std::memcpy(frame.data(), hdr.data(), 4);
                                    read_rest(len - 4);
                                });
    }

    // Read the remaining payload after the 4-byte length.
    // 4 バイトの長ヘッダ以降のペイロードを読み込む。
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
                // Try decoding a bus::Message from the full frame.
                // フルフレームから bus::Message をデコード。
                try
                {
                    bus::Message msg;
                    bus::decode(frame.data(), frame.size(), msg);

                    // Log the message id and options for quick troubleshooting.
                    // メッセージIDとオプションを記録し、解析を容易にする。
                    Logger::debug("[bus] rx MID=" + msg.messageID +
                                  " options=" + std::to_string(msg.options));

                    // Expect Poseidon Query with a binary "packet" field.
                    // "packet" バイナリ引数を持つ Poseidon Query を期待。
                    if (msg.options == 0 && msg.messageID == "Poseidon Query")
                    {
                        auto it = msg.args_map.find("packet");
                        if (it != msg.args_map.end() && it->second.type == bus::ValueType::Binary)
                        {
                            if (on_query)
                            {
                                const auto& blob = it->second.bin;
                                Logger::info(banner("RX←THANATOS", "Thanatos Query", blob.size()));
                                if (blob.size() >= 4 && (blob[0] | (blob[1] << 8)) >= 0x0001)
                                {
                                    Logger::info("(raw) no RO header");
                                }
                                Logger::info("\n" + hex_dump(blob.data(), blob.size()));

                                on_query(it->second.bin);
                            }
                        }
                    }
                }
                catch (const std::exception& e)
                {
                    // Decoding errors are non-fatal; we close & accept again.
                    // デコード失敗は致命的ではない。クローズして再受け付け。
                    Logger::warn(std::string("[bus] decode error: ") + e.what());
                }
            });
    }

    // Close current client socket and resume accepting new ones.
    // 現在のソケットを閉じ、新規 accept を再開する。
    void recycle()
    {
        boost::system::error_code ec;
        if (sock)
        {
            sock->shutdown(tcp::socket::shutdown_both, ec);
            sock->close(ec);
        }
        async_accept();
    }

    // Encode and send a "Poseidon Reply" message with a binary payload.
    // バイナリペイロード付きの "Poseidon Reply" をエンコードして送信する。
    void send_reply_bin(const std::vector<uint8_t>& payload)
    {
        if (!sock || !sock->is_open())
        {
            // No active client; drop the reply silently.
            // アクティブなクライアントがないため応答は破棄。
            Logger::debug("[bus] no client to reply");
            return;
        }

        Logger::debug(std::string("[bus] tx MID=Thanatos Reply len=") +
                      std::to_string(payload.size()));

        bus::Message msg;
        msg.options = 0;  // "map" style message (see BusProtocol)
        msg.messageID = "Poseidon Reply";
        bus::Value v;
        v.type = bus::ValueType::Binary;  // binary field
        v.bin = payload;
        msg.args_map.emplace("packet", std::move(v));

        // Keep buffer alive until async_write completes by capturing shared_ptr.
        // 非同期書き込み完了までバッファを保持するため shared_ptr を捕捉。
        auto buf = std::make_shared<std::vector<uint8_t>>(bus::encode(msg));

        boost::asio::async_write(
            *sock, boost::asio::buffer(*buf),
            [this, buf](boost::system::error_code ec, std::size_t)
            {
                if (ec)
                {
                    Logger::warn(std::string("[bus] write error: ") + ec.message());
                }
                else
                {
                    Logger::debug("[bus] reply sent successfully, closing connection");
                }
                // Closes the connection after sending the response
                recycle();
            });
    }
};

// Public façade: forwards into Impl.
// 公開ファサード：Impl へ委譲。
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

// Register a callback to receive raw payloads extracted from Poseidon Query.
// Poseidon Query から抽出した生ペイロードを受け取るコールバックを登録。
void QueryServer::onQuery(std::function<void(std::vector<uint8_t>)> cb)
{
    impl_->on_query = std::move(cb);
}

// Convenience wrapper to send a Poseidon Reply with logging of the first bytes.
// 先頭バイトをログに出しつつ Poseidon Reply を送信するためのラッパ。
void QueryServer::sendReply(const std::vector<uint8_t>& gg_reply)
{
    auto head_bus = arkan::thanatos::shared::hex::hex(gg_reply.data(),
                                                      std::min<std::size_t>(gg_reply.size(), 32));
    Logger::debug("[bus] tx Thanatos Reply payload head32=" + head_bus +
                  " len=" + std::to_string(gg_reply.size()));

    Logger::info(banner("TX→THANATOS", "Thanatos Reply", gg_reply.size()));
    Logger::info("\n" + hex_dump(gg_reply.data(), gg_reply.size()));

    impl_->send_reply_bin(gg_reply);
}

// Return "host:port" for logging or UI.
// ログや UI 用の "host:port" 文字列表現を返す。
std::string QueryServer::endpoint_description() const
{
    return impl_->host + ":" + std::to_string(impl_->port);
}

}  // namespace arkan::thanatos::interface::query