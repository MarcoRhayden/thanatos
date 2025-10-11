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

namespace arkan::thanatos::infrastructure::net::asio_impl
{

/**
 * AsioTcpClient
 * -----------------------------------------------------------------------------
 * Thin TCP client wrapper with:
 *     - serialized handlers via strand (thread-safe internal state),
 *     - auto-reconnect (exponential backoff),
 *     - per-operation watchdog timers (connect/read/write),
 *     - backpressure on the write queue,
 *     - user callbacks for data/error.
 *
 * 軽量な TCP クライアント。以下を提供:
 *     - strand による直列化（内部状態はスレッド安全）
 *     - 自動再接続（指数バックオフ）
 *     - 接続/受信/送信の各監視タイマ
 *     - 送信キューのバックプレッシャ
 *     - データ/エラーのユーザコールバック
 */
class AsioTcpClient : public std::enable_shared_from_this<AsioTcpClient>
{
   public:
    /// Data callback signature (zero-copy view).
    /// データ到着時のコールバック（ゼロコピー風ビュー）。
    using DataHandler = std::function<void(std::span<const std::uint8_t>)>;
    /// Error callback (portable std::error_code).
    /// エラー通知用（移植性のある std::error_code）。
    using ErrorHandler = std::function<void(const std::error_code&)>;

    /**
     * Construct on an existing io_context. A strand is created to serialize all handlers.
     * 既存の io_context 上に生成。ハンドラ直列化のため strand を作成。
     */
    explicit AsioTcpClient(asio::io_context& io)
        : io_(io), socket_(io), strand_(asio::make_strand(io))
    {
    }

    /**
     * Begin async resolve/connect and enter auto-reconnect loop on failure.
     * 非同期解決/接続を開始。失敗時は自動再接続ループへ。
     */
    void connect(const std::string& host, std::uint16_t port);

    /**
     * Queue bytes for async write; honors backpressure limits.
     * 非同期送信のためにバイト列をキューへ投入。バックプレッシャ制限に従う。
     */
    void send(std::span<const std::uint8_t> bytes);

    /**
     * Close socket and cancel timers; safe to call from any thread.
     * ソケットを閉じ、各タイマをキャンセル。どのスレッドからでも安全に呼べる。
     */
    void close();

    /// Install data handler; invoked on the strand.
    /// データコールバックを設定（strand 上で実行）。
    void on_data(DataHandler h)
    {
        on_data_ = std::move(h);
    }

    /// Install error handler; invoked on the strand.
    /// エラーコールバックを設定（strand 上で実行）。
    void on_error(ErrorHandler h)
    {
        on_error_ = std::move(h);
    }

   private:
    /// Upper bound for queued outbound packets to protect memory.
    /// メモリ保護のための送信キュー上限。
    static constexpr std::size_t kMaxWriteQueue = 1024;

    // Implementation steps (all executed on the strand).
    // 実装ステップ（すべて strand 上で実行）。
    void do_resolve_and_connect(const std::string& host, std::uint16_t port);
    void do_read();
    void do_write();
    void fail(const boost::system::error_code& ec);
    void schedule_reconnect();

    // ============================
    // Transport & execution
    // トランスポートと実行基盤
    // ============================
    asio::io_context& io_;
    tcp::socket socket_;
    /// Serializes all handlers to avoid data races.
    /// すべてのハンドラを直列化し競合を防ぐ。
    asio::strand<asio::any_io_executor> strand_;

    // ============================
    // Buffers & queues
    // バッファとキュー
    // ============================
    /// Reused buffer for async_read_some().
    /// async_read_some() 用の再利用バッファ。
    std::vector<std::uint8_t> read_buf_{};
    /// Outbound packet queue (FIFO).
    /// 送信パケットの待ち行列（FIFO）。
    std::deque<std::vector<std::uint8_t>> write_q_;

    // ============================
    // User callbacks
    // ユーザコールバック
    // ============================
    DataHandler on_data_;
    ErrorHandler on_error_;

    // ============================
    // Auto-reconnect & watchdogs
    // 自動再接続と監視タイマ
    // ============================
    /// Last requested endpoint (for reconnect).
    /// 再接続用の接続先情報。
    std::string host_;
    std::uint16_t port_{0};

    /// Consecutive reconnect attempts counter.
    /// 連続再接続回数カウンタ。
    int reconnect_attempts_{0};

    /// Per-operation timers (connect/read/write) and backoff timer.
    /// 各操作（接続/受信/送信）の監視タイマおよびバックオフ用タイマ。
    std::unique_ptr<asio::steady_timer> connect_timer_;
    std::unique_ptr<asio::steady_timer> read_timer_;
    std::unique_ptr<asio::steady_timer> write_timer_;
    std::unique_ptr<asio::steady_timer> reconnect_timer_;
};

}  // namespace arkan::thanatos::infrastructure::net::asio_impl
