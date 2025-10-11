#include "infrastructure/net/asio/AsioTcpClient.hpp"

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <chrono>
#include <string>
#include <vector>

#include "infrastructure/log/Logger.hpp"

using arkan::thanatos::infrastructure::log::Logger;

namespace arkan::thanatos::infrastructure::net::asio_impl
{

// ============================================================================
// Robustness knobs (timeouts, backoff, queue limits).
// しきい値（タイムアウト・バックオフ・キュー制限）。運用で調整しやすいよう集約。
// ============================================================================
constexpr auto CONNECT_TIMEOUT = std::chrono::seconds(10);  // Connect watchdog
                                                            // 接続監視タイマ
constexpr auto READ_TIMEOUT = std::chrono::seconds(60);     // Read watchdog
                                                            // 受信監視
constexpr auto WRITE_TIMEOUT = std::chrono::seconds(30);    // Write watchdog
                                                            // 送信監視
constexpr int MAX_RECONNECT_ATTEMPTS = 10;                  // Give up after N tries
                                                            // 再接続最大回数
constexpr int MAX_RECONNECT_DELAY = 60;                     // Cap exponential delay
                                                            // バックオフ上限(秒)

// ============================================================================
// AsioTcpClient — auto reconnect + watch-dog timers on a strand.
// 共有ストランド上で再接続と監視タイマを管理する TCP クライアント。
// - すべてのハンドラは strand_ で直列化 → 内部状態の競合を排除。
// ============================================================================
void AsioTcpClient::connect(const std::string& host, std::uint16_t port)
{
    auto self = shared_from_this();
    // Schedule onto the strand so user-thread calls cannot race with I/O callbacks.
    // ユーザスレッドからの呼び出しをストランドへ投げ、I/O コールバックと競合しないようにする。
    asio::dispatch(strand_,
                   [this, self, host, port]
                   {
                       host_ = host;
                       port_ = port;
                       reconnect_attempts_ = 0;
                       do_resolve_and_connect(host, port);
                   });
}

void AsioTcpClient::do_resolve_and_connect(const std::string& host, std::uint16_t port)
{
    auto self = shared_from_this();

    Logger::info("[client] Connecting to " + host + ":" + std::to_string(port) + "...");

    // Sync resolve is fine here; errors fall back to the reconnect loop.
    // ここは同期解決で十分。失敗時は再接続ループへ。
    boost::system::error_code ec;
    tcp::resolver resolver(io_);
    auto results = resolver.resolve(host, std::to_string(port), ec);
    if (ec)
    {
        Logger::error("[client] DNS resolution failed: " + ec.message());
        schedule_reconnect();
        return;
    }

    // Arm a connect watchdog; if the handshake stalls, we cancel the socket.
    // 接続監視をセット。ハンドシェイクが停滞したらソケットを cancel。
    connect_timer_ = std::make_unique<asio::steady_timer>(io_);
    connect_timer_->expires_after(CONNECT_TIMEOUT);
    connect_timer_->async_wait(
        [this, self](const boost::system::error_code& ec)
        {
            if (!ec)
            {
                Logger::warn("[client] Connect timeout");
                boost::system::error_code cancel_ec;
                socket_.cancel(cancel_ec);
            }
        });

    // async_connect chooses the first successful endpoint from results.
    // async_connect は到達可能なエンドポイントに順次試行する。
    asio::async_connect(
        socket_, results,
        asio::bind_executor(
            strand_,
            [this, self](const boost::system::error_code& bec, const tcp::endpoint& endpoint)
            {
                // Disarm the watchdog regardless of success/failure.
                // 成否に関わらず監視タイマを解除。
                if (connect_timer_)
                {
                    connect_timer_->cancel();
                    connect_timer_.reset();
                }

                if (bec)
                {
                    // Distinguish timeout-cancel from other failures for clearer logs.
                    // タイムアウト起因の cancel と通常エラーをログで区別。
                    if (bec == asio::error::operation_aborted)
                    {
                        Logger::warn("[client] Connection attempt timed out");
                    }
                    else
                    {
                        Logger::error("[client] Connection failed: " + bec.message());
                    }
                    schedule_reconnect();
                    return;
                }

                Logger::info("[client] Connected to " + endpoint.address().to_string() + ":" +
                             std::to_string(endpoint.port()));

                // Reset backoff on a healthy connection.
                // 接続成功でバックオフをリセット。
                reconnect_attempts_ = 0;

                // Practical defaults: disable Nagle, enable keepalive for liveness.
                // 実用的な既定：Nagle 無効・KeepAlive 有効。
                boost::system::error_code ec;
                socket_.set_option(tcp::no_delay(true), ec);
                socket_.set_option(boost::asio::socket_base::keep_alive(true), ec);

                // Allocate once; re-use for read_some().
                // 一度だけ確保して使い回す。
                read_buf_.resize(8192);
                do_read();
            }));
}

void AsioTcpClient::schedule_reconnect()
{
    // Hard stop after too many consecutive failures (surface error upstream).
    // 連続失敗が閾値超え → 上位へエラーを返して停止。
    if (reconnect_attempts_ >= MAX_RECONNECT_ATTEMPTS)
    {
        Logger::error("[client] Max reconnection attempts reached, giving up");
        fail(boost::system::error_code(ETIMEDOUT, boost::system::generic_category()));
        return;
    }

    reconnect_attempts_++;

    // Exponential backoff (1,2,4,8,...) seconds, clamped to MAX_RECONNECT_DELAY.
    // 指数バックオフ（1,2,4,8...秒）。上限で打ち止め。
    int delay =
        std::min(static_cast<int>(std::pow(2, reconnect_attempts_ - 1)), MAX_RECONNECT_DELAY);

    Logger::info("[client] Reconnecting in " + std::to_string(delay) + "s (attempt " +
                 std::to_string(reconnect_attempts_) + "/" +
                 std::to_string(MAX_RECONNECT_ATTEMPTS) + ")...");

    reconnect_timer_ = std::make_unique<asio::steady_timer>(io_);
    reconnect_timer_->expires_after(std::chrono::seconds(delay));
    reconnect_timer_->async_wait(
        [this, self = shared_from_this()](const boost::system::error_code& ec)
        {
            if (!ec)
            {
                do_resolve_and_connect(host_, port_);
            }
        });
}

void AsioTcpClient::send(std::span<const std::uint8_t> bytes)
{
    auto self = shared_from_this();
    // Copy into an owning buffer; the caller may outlive the async write.
    // 呼び出し元のバッファ寿命に依存しないよう所有バッファへコピー。
    std::vector<std::uint8_t> copy(bytes.begin(), bytes.end());

    asio::dispatch(strand_,
                   [this, self, buf = std::move(copy)]() mutable
                   {
                       write_q_.push_back(std::move(buf));

                       // Backpressure guard: if producers outrun the socket, shed oldest half.
                       // バックプレッシャ：送信元が暴走した場合は古い半分を捨てて守る。
                       if (write_q_.size() > kMaxWriteQueue)
                       {
                           Logger::error("[client] Write queue overflow (" +
                                         std::to_string(write_q_.size()) + " packets)");
                           while (write_q_.size() > kMaxWriteQueue / 2)
                           {
                               write_q_.pop_front();
                           }
                           Logger::warn("[client] Dropped old packets, queue size now: " +
                                        std::to_string(write_q_.size()));
                       }

                       // If idle, start the writer; otherwise the in-flight write will chain.
                       // アイドルなら送信開始。そうでなければ進行中の完了時に連結される。
                       if (write_q_.size() == 1) do_write();
                   });
}

void AsioTcpClient::do_write()
{
    if (write_q_.empty() || !socket_.is_open()) return;

    // Per-write watchdog: cancels the socket if the kernel never completes the write.
    // 書き込み監視：カーネルが返ってこない異常を検知 → ソケット cancel。
    write_timer_ = std::make_unique<asio::steady_timer>(io_);
    write_timer_->expires_after(WRITE_TIMEOUT);
    write_timer_->async_wait(
        [this, self = shared_from_this()](const boost::system::error_code& ec)
        {
            if (!ec)
            {
                Logger::warn("[client] Write timeout");
                boost::system::error_code cancel_ec;
                socket_.cancel(cancel_ec);
            }
        });

    auto self = shared_from_this();
    asio::async_write(
        socket_, asio::buffer(write_q_.front()),
        asio::bind_executor(strand_,
                            [this, self](const boost::system::error_code& bec, std::size_t /*n*/)
                            {
                                // Always disarm the watchdog first.
                                // まず監視タイマを停止。
                                if (write_timer_)
                                {
                                    write_timer_->cancel();
                                    write_timer_.reset();
                                }

                                if (bec)
                                {
                                    // Timeout vs other I/O error for clearer ops signals.
                                    // タイムアウトとその他 I/O エラーを分けて通知。
                                    if (bec == asio::error::operation_aborted)
                                    {
                                        Logger::warn("[client] Write operation timed out");
                                    }
                                    else
                                    {
                                        Logger::error("[client] Write error: " + bec.message());
                                    }

                                    // Close -> schedule reconnect to restore the pipe.
                                    // クローズしてパイプを再構築。
                                    close();
                                    schedule_reconnect();
                                    return;
                                }

                                // Pop written packet and continue if more remain.
                                // 送信済みを取り除き、残があれば継続。
                                write_q_.pop_front();
                                if (!write_q_.empty()) do_write();
                            }));
}

void AsioTcpClient::do_read()
{
    if (!socket_.is_open()) return;

    // Read watchdog: protects against silent stalls (peer stop sending).
    // 受信監視：沈黙停滞を検知。相手が送信停止した場合の保険。
    read_timer_ = std::make_unique<asio::steady_timer>(io_);
    read_timer_->expires_after(READ_TIMEOUT);
    read_timer_->async_wait(
        [this, self = shared_from_this()](const boost::system::error_code& ec)
        {
            if (!ec)
            {
                Logger::warn("[client] Read timeout");
                boost::system::error_code cancel_ec;
                socket_.cancel(cancel_ec);
            }
        });

    auto self = shared_from_this();
    socket_.async_read_some(
        asio::buffer(read_buf_),
        asio::bind_executor(strand_,
                            [this, self](const boost::system::error_code& bec, std::size_t n)
                            {
                                // Disarm watchdog before branching.
                                // 分岐前に監視タイマ解除。
                                if (read_timer_)
                                {
                                    read_timer_->cancel();
                                    read_timer_.reset();
                                }

                                if (bec)
                                {
                                    // Normalize common cases for friendlier logs.
                                    // 典型的ケースを整形して読みやすく。
                                    if (bec == asio::error::eof)
                                    {
                                        Logger::info("[client] Connection closed by server");
                                    }
                                    else if (bec == asio::error::connection_reset)
                                    {
                                        Logger::warn("[client] Connection reset by server");
                                    }
                                    else if (bec == asio::error::operation_aborted)
                                    {
                                        Logger::debug("[client] Read operation aborted (timeout)");
                                    }
                                    else
                                    {
                                        Logger::error("[client] Read error: " + bec.message());
                                    }

                                    // Reset the transport and try again via backoff.
                                    // トランスポートを畳んでバックオフ再接続へ。
                                    close();
                                    schedule_reconnect();
                                    return;
                                }

                                // Deliver bytes to the consumer callback, then keep reading.
                                // コールバックへ渡してから読み取りループ継続。
                                if (n > 0 && on_data_)
                                {
                                    on_data_(std::span<const std::uint8_t>(read_buf_.data(), n));
                                }

                                do_read();
                            }));
}

void AsioTcpClient::close()
{
    auto self = shared_from_this();
    // Serialize shutdown/cleanup on the strand to avoid racing timers & socket.
    // タイマとソケット操作の競合を避けるため、ストランド上でシリアライズ。
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

                       // Best-effort cancel of all timers; harmless if not armed.
                       // 全タイマをベストエフォートでキャンセル。未起動でも無害。
                       if (connect_timer_) connect_timer_->cancel();
                       if (read_timer_) read_timer_->cancel();
                       if (write_timer_) write_timer_->cancel();
                       if (reconnect_timer_) reconnect_timer_->cancel();
                   });
}

void AsioTcpClient::fail(const boost::system::error_code& ec)
{
    // Surface a portable std::error_code upstream for app-level policy.
    // 上位レイヤが扱いやすい std::error_code に変換して通知。
    if (on_error_) on_error_(std::error_code(ec.value(), std::generic_category()));
}

}  // namespace arkan::thanatos::infrastructure::net::asio_impl
