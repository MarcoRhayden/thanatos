#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "application/ports/query/IQueryServer.hpp"
#include "shared/Hex.hpp"

namespace boost
{
namespace asio
{
class io_context;
}
}  // namespace boost

namespace arkan::thanatos::interface::query
{

// Concrete implementation of the application port that exposes a lightweight
// binary query/response endpoint over TCP using a custom "PSDN" frame.
// The goal is to decouple network I/O from domain logic while keeping the
// transport format explicit and testable.
// アプリケーションポートの具象実装。独自の「PSDN」フレームで TCP 上の
// 軽量な問い合わせ/応答エンドポイントを提供する。ネットワーク I/O と
// ドメインロジックを分離しつつ、転送形式を明示的かつテスト可能に保つ。

// Wire format (big-endian length after the type):
//   magic[4]      = "PSDN"
//   type (u16)    = 0x0051 Query / 0x0052 Reply
//   length (u32)  = N (number of bytes in payload)
//   payload[N]
// ワイヤ形式（type の後に長さのビッグエンディアン）:
//   magic[4]      = "PSDN"
//   type (u16)    = 0x0051 Query / 0x0052 Reply
//   length (u32)  = N（ペイロード長）
//   payload[N]

// QueryServer owns only the orchestration and lifetime of the network side;
// the actual interpretation of payloads is delegated via onQuery() callbacks.
// QueryServer はネットワーク側の制御/寿命のみを担当し、ペイロードの解釈は
// onQuery() のコールバックに委譲する。
class QueryServer final : public application::ports::query::IQueryServer
{
   public:
    // Construct a server bound to (host, port) and running on the given io_context.
    // Ownership of io_context stays with the caller; QueryServer never calls run().
    // 指定 io_context 上で (host, port) にバインドするサーバを構築。
    // io_context の所有権は呼び出し側にあり、QueryServer は run() を呼ばない。
    QueryServer(boost::asio::io_context& io, const std::string& host, std::uint16_t port);
    ~QueryServer();

    // Begin accepting connections and reading frames. Idempotent.
    // 接続受け付けとフレーム読み取りを開始（多重呼び出しに寛容）。
    void start() override;

    // Stop accepting and close the active socket if any. Safe to call repeatedly.
    // 受け付け停止し、アクティブなソケットがあれば閉じる（再呼び出し可）。
    void stop() override;

    // Register a handler invoked for each decoded Query payload (type=0x0051).
    // The handler receives only the binary payload, not the PSDN header.
    // デコード済み Query（type=0x0051）のペイロードごとに呼ばれるハンドラを登録。
    // ハンドラには PSDN ヘッダを除いたバイナリのみが渡される。
    void onQuery(std::function<void(std::vector<std::uint8_t>)> cb) override;

    // Send a Reply (type=0x0052) to the currently connected client, if any.
    // The method is transport-safe and logs the first bytes for debugging.
    // 現在のクライアントがあれば Reply（type=0x0052）を送信。
    // 転送は安全に行われ、先頭バイトをデバッグ用に記録する。
    void sendReply(const std::vector<std::uint8_t>& gg_reply) override;

    // Human-friendly "host:port" string for logs or UI.
    // ログや UI 向けの「host:port」表記を返す。
    std::string endpoint_description() const override;

   private:
    // Pimpl hides Boost.Asio members and keeps this header stable.
    // Pimpl により Boost.Asio の詳細を隠蔽し、ヘッダ安定性を保つ。
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace arkan::thanatos::interface::query
