#include "AsioTcpServer.hpp"

#include <array>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <deque>
#include <span>
#include <stdexcept>

#include "application/ports/net/ISession.hpp"
#include "infrastructure/log/Logger.hpp"

using arkan::thanatos::infrastructure::log::Logger;
namespace ports = arkan::thanatos::application::ports::net;

namespace arkan::thanatos::infrastructure::net::asio_impl
{

// ============================================================================
// Balanced configuration
// - Keep the server resilient without being trigger-happy on timeouts.
// - Write timeout remains to detect stalled peers under backpressure.
// バランス設定
// - タイムアウト過敏にならない堅牢性を重視。
// - バックプレッシャ下での停滞検知のため送信タイムアウトは維持。
// ============================================================================
constexpr auto WRITE_TIMEOUT = std::chrono::seconds(30);

// ============================================================================
// TcpSession — minimal, robust session wrapper
// - Single strand to serialize all operations
// - Moderate write queue protection
// - No read timeout (RO client may idle)
// TcpSession — 最小限で堅牢なセッションラッパ
// - strand により全操作を直列化
// - 送信キューの適度な保護
// - 読み取りタイムアウトなし（RO クライアントはアイドルになり得る）
// ============================================================================
class TcpSession final : public ports::ISession, public std::enable_shared_from_this<TcpSession>
{
   public:
    TcpSession(tcp::socket socket, std::shared_ptr<ports::IConnectionHandler> handler)
        : socket_(std::move(socket)),
          strand_(socket_.get_executor()),
          handler_(std::move(handler)),
          write_timer_(socket_.get_executor())
    {
        // Enable system-level TCP keepalive and disable Nagle (lower latency).
        // システム標準の TCP keepalive を有効化し、Nagle を無効化（低遅延化）。
        boost::system::error_code ec;
        socket_.set_option(asio::socket_base::keep_alive(true), ec);
        socket_.set_option(tcp::no_delay(true), ec);
    }

    // Start async read loop. All callbacks run on the strand.
    // 非同期読み取りループ開始。全コールバックは strand 上で動作。
    void start()
    {
        auto self = shared_from_this();
        asio::dispatch(strand_, [this, self] { do_read(); });
    }

    // Queue data for write; enforce moderate backpressure limit.
    // データを送信キューへ投入。適度なバックプレッシャ制限を適用。
    void send(std::span<const std::uint8_t> data) override
    {
        auto self = shared_from_this();
        asio::dispatch(strand_,
                       [this, self, bytes = Buffer(data.begin(), data.end())]() mutable
                       {
                           write_queue_.push_back(std::move(bytes));

                           // Guard against unbounded memory growth. If the peer cannot
                           // drain quickly enough, fail fast to protect the process.
                           // 無制限なメモリ増大を防止。対向が十分に捌けない場合は保護のため早期終了。
                           if (write_queue_.size() > 1000)
                           {
                               Logger::error("[net] Write queue overflow (" +
                                             std::to_string(write_queue_.size()) +
                                             " packets), closing connection: " + remote_endpoint());
                               do_close(std::error_code(ENOBUFS, std::generic_category()));
                               return;
                           }

                           if (write_queue_.size() == 1) do_write();
                       });
    }

    // Gentle, idempotent close request. Safe across threads.
    // 穏当かつ冪等なクローズ要求。スレッド安全。
    void close() override
    {
        auto self = shared_from_this();
        asio::dispatch(strand_, [this, self] { do_close(std::error_code()); });
    }

    // Human-friendly remote endpoint string for logs/diagnostics.
    // ログ/診断向けの可読な相手エンドポイント表記。
    std::string remote_endpoint() const override
    {
        boost::system::error_code ec;
        auto ep = socket_.remote_endpoint(ec);
        if (ec) return "[closed]";
        return ep.address().to_string() + ":" + std::to_string(ep.port());
    }

   private:
    // Read loop: no read timeout by design. RO clients often idle between frames.
    // 読み取りループ：仕様として読み取りタイムアウトなし。RO
    // クライアントはフレーム間で待機することが多い。
    void do_read()
    {
        if (!socket_.is_open()) return;

        auto self = shared_from_this();
        socket_.async_read_some(
            asio::buffer(read_buf_),
            asio::bind_executor(strand_,
                                [this, self](const boost::system::error_code& bec, std::size_t n)
                                {
                                    if (bec)
                                    {
                                        handle_read_error(bec);
                                        return;
                                    }

                                    // Forward raw bytes upstream.
                                    // 受信バイト列を上位へ通知。
                                    handler_->on_data(
                                        self, std::span<const std::uint8_t>(read_buf_.data(), n));

                                    // Continue reading.
                                    // 継続読み取り。
                                    do_read();
                                }));
    }

    // Drain the write queue; keep a watchdog to detect stalls.
    // 書き込みキューを吐き出す。停滞検知のためウォッチドッグを保持。
    void do_write()
    {
        if (write_queue_.empty() || !socket_.is_open()) return;

        // Only a write timeout is enforced (helps noticing dead connections).
        // 送信タイムアウトのみを適用（死活検知に有効）。
        write_timer_.expires_after(WRITE_TIMEOUT);
        write_timer_.async_wait(
            [this, self = shared_from_this()](const boost::system::error_code& ec)
            {
                if (!ec)
                {
                    Logger::warn("[net] Write timeout: " + remote_endpoint());
                    boost::system::error_code cancel_ec;
                    socket_.cancel(cancel_ec);
                }
            });

        auto self = shared_from_this();
        asio::async_write(
            socket_, asio::buffer(write_queue_.front()),
            asio::bind_executor(strand_,
                                [this, self](const boost::system::error_code& bec, std::size_t)
                                {
                                    write_timer_.cancel();

                                    if (bec)
                                    {
                                        handle_write_error(bec);
                                        return;
                                    }

                                    write_queue_.pop_front();
                                    if (!write_queue_.empty()) do_write();
                                }));
    }

    // Normalize read errors and notify upstream once.
    // 読み取りエラーを正規化して上位へ一度だけ通知。
    void handle_read_error(const boost::system::error_code& bec)
    {
        std::error_code ec(bec.value(), std::generic_category());

        if (bec == asio::error::eof)
        {
            Logger::debug("[net] Connection closed by peer: " + remote_endpoint());
        }
        else if (bec == asio::error::connection_reset)
        {
            Logger::warn("[net] Connection reset: " + remote_endpoint());
        }
        else if (bec == asio::error::operation_aborted)
        {
            Logger::debug("[net] Read operation aborted: " + remote_endpoint());
        }
        else
        {
            Logger::warn("[net] Read error: " + bec.message() + " - " + remote_endpoint());
        }

        handler_->on_disconnect(shared_from_this(), ec);
    }

    // Normalize write errors and notify upstream once.
    // 書き込みエラーを正規化して上位へ一度だけ通知。
    void handle_write_error(const boost::system::error_code& bec)
    {
        std::error_code ec(bec.value(), std::generic_category());

        if (bec == asio::error::operation_aborted)
        {
            Logger::warn("[net] Write timeout reached: " + remote_endpoint());
        }
        else
        {
            Logger::warn("[net] Write error: " + bec.message() + " - " + remote_endpoint());
        }

        handler_->on_disconnect(shared_from_this(), ec);
    }

    // Best-effort close; cancel timers; notify if a reason was provided.
    // ベストエフォートでクローズ。タイマを停止し、理由があれば通知。
    void do_close(const std::error_code& reason)
    {
        if (!socket_.is_open()) return;

        boost::system::error_code ec;
        socket_.shutdown(asio::socket_base::shutdown_both, ec);
        socket_.close(ec);

        write_timer_.cancel();

        if (reason)
        {
            handler_->on_disconnect(shared_from_this(), reason);
        }
    }

    tcp::socket socket_;
    // Serialize all operations; prevents data races without coarse global locks.
    // 全操作を直列化。大域ロックに頼らず競合を防止。
    asio::strand<tcp::socket::executor_type> strand_;
    // Slightly larger read buffer to reduce syscalls for small bursts.
    // 小規模バースト時のシステムコール削減を狙い、やや大きめの読み取りバッファ。
    std::array<std::uint8_t, 8192> read_buf_{};
    std::deque<Buffer> write_queue_;
    std::shared_ptr<ports::IConnectionHandler> handler_;

    // Only a write watchdog is kept (balanced approach).
    // 書き込み監視のみ保持（バランス重視）。
    asio::steady_timer write_timer_;
};

// ============================================================================
// AsioTcpServerImpl — accept loop + session factory
// - Validates bind IP and configures reuse/v6-only properly
// - Hands new sockets to TcpSession
// AsioTcpServerImpl — 受け付けループ + セッション生成
// - bind IP を検証し、reuse/v6-only を適切に設定
// - 新規ソケットを TcpSession に委譲
// ============================================================================
class AsioTcpServerImpl final : public ports::ITcpServer
{
   public:
    AsioTcpServerImpl(asio::io_context& io, std::uint16_t port,
                      std::shared_ptr<ports::IConnectionHandler> h, const std::string& bind_ip)
        : io_(io), acceptor_(io), port_(port), handler_(std::move(h)), running_(false)
    {
        using asio::ip::address;
        boost::system::error_code ec;

        // Parse bind address early; fail fast on configuration mistakes.
        // bind アドレスを早期解析し、設定ミスは即時失敗。
        address addr = asio::ip::make_address(bind_ip, ec);
        if (ec)
        {
            throw std::runtime_error("Invalid bind_ip '" + bind_ip + "': " + ec.message());
        }

        tcp::endpoint ep(addr, port_);

        // Open acceptor and configure socket-level options BEFORE bind().
        // bind() 前にアセプタを open し、ソケットオプションを設定。
        acceptor_.open(ep.protocol(), ec);
        if (ec) throw std::runtime_error("acceptor.open: " + ec.message());

#if defined(_WIN32)
        // On Windows, enforce per-process exclusivity for (ip,port) using SO_EXCLUSIVEADDRUSE.
        // Windows では SO_EXCLUSIVEADDRUSE を使い、(ip,port) の排他利用を強制する。
        {
            BOOL opt = TRUE;
            ::setsockopt(acceptor_.native_handle(), SOL_SOCKET, SO_EXCLUSIVEADDRUSE,
                         reinterpret_cast<const char*>(&opt), sizeof(opt));
        }
        // Also make sure SO_REUSEADDR is disabled so exclusivity isn't undermined.
        // 併せて SO_REUSEADDR を無効化して、排他制御を損なわないようにする。
        {
            BOOL opt = FALSE;
            ::setsockopt(acceptor_.native_handle(), SOL_SOCKET, SO_REUSEADDR,
                         reinterpret_cast<const char*>(&opt), sizeof(opt));
        }
#else
        // On POSIX, reuse_address helps with restarts (TIME_WAIT) and does NOT allow two
        // processes to bind the same (ip,port) simultaneously.
        // POSIX では再起動(TIME_WAIT)対策として reuse_address を有効化しても、
        // 複数プロセスによる同一 (ip,port) 同時バインドは許可されない。
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
#endif

        // If binding to IPv6, allow dual-stack when OS permits.
        // IPv6 にバインドする場合、OS が許せばデュアルスタックを許可。
        if (addr.is_v6())
        {
            boost::system::error_code ec2;
            acceptor_.set_option(asio::ip::v6_only(false), ec2);
        }

        // Bind and start listening.
        // bind して listen を開始。
        acceptor_.bind(ep, ec);
        if (ec) throw std::runtime_error("acceptor.bind(" + bind_ip + "): " + ec.message());

        acceptor_.listen(asio::socket_base::max_listen_connections, ec);
        if (ec) throw std::runtime_error("acceptor.listen: " + ec.message());
    }

    // Enter accept loop.
    // 受け付けループへ突入。
    void start() override
    {
        running_ = true;
        do_accept();
    }

    // Stop accepting new connections and close the acceptor.
    // 新規接続の受け付けを止め、アセプタをクローズ。
    void stop() override
    {
        running_ = false;
        boost::system::error_code ec;
        acceptor_.cancel(ec);
        acceptor_.close(ec);
        Logger::info("[net] TCP Server stopped");
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
    // Single-shot accept followed by re-arming to keep the server alive.
    // 1 回の accept 後に再度待機し、サーバを生かし続ける。
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
                else
                {
                    Logger::warn("[net] Accept error: " + bec.message());
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

/* ------------------------ Factories ------------------------ */

// Create a server bound to the given IP/port.
// 指定 IP/ポートにバインドしたサーバを生成。
std::unique_ptr<ports::ITcpServer> MakeTcpServer(asio::io_context& io, std::uint16_t port,
                                                 std::shared_ptr<ports::IConnectionHandler> handler,
                                                 const std::string& bind_ip)
{
    return std::unique_ptr<ports::ITcpServer>(
        new AsioTcpServerImpl(io, port, std::move(handler), bind_ip));
}

// Convenience overload: bind to 0.0.0.0 (all interfaces).
// 省略版：0.0.0.0（全インタフェース）にバインド。
std::unique_ptr<ports::ITcpServer> MakeTcpServer(asio::io_context& io, std::uint16_t port,
                                                 std::shared_ptr<ports::IConnectionHandler> handler)
{
    return MakeTcpServer(io, port, std::move(handler), std::string("0.0.0.0"));
}

}  // namespace arkan::thanatos::infrastructure::net::asio_impl
