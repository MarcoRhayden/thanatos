#include "interface/query/QueryHandler.hpp"

#include "interface/query/QueryProtocol.hpp"

using arkan::thanatos::interface::query::wire::decode_blob;
using arkan::thanatos::interface::query::wire::encode_blob;
using arkan::thanatos::interface::query::wire::frame;
using arkan::thanatos::interface::query::wire::MSG_THANATOS_QUERY;
using arkan::thanatos::interface::query::wire::MSG_THANATOS_REPLY;
using arkan::thanatos::interface::query::wire::r16;

namespace arkan::thanatos::interface::query
{

// ----------------------------------------------------------------------------
// Called when a TCP peer connects to the Query endpoint.
// Keeps a weak association (by raw pointer key) between the socket/session and
// a small connection state (buffer).
// クエリエンドポイントに TCP ピアが接続したタイミングで呼ばれる。
// ソケット/セッションと小さな接続状態（バッファ）を生ポインタキーで関連付ける。
void QueryHandler::on_connect(std::shared_ptr<ports::ISession> s)
{
    Logger::info("[query] connect: " + s->remote_endpoint());
    peers_.emplace(s.get(), Conn{std::move(s), {}});
}

// ----------------------------------------------------------------------------
// Called when a peer disconnects or errors out.
// Cleans up the bookkeeping map entry.
// ピア切断/エラー時に呼ばれる。管理用マップのエントリを削除する。
void QueryHandler::on_disconnect(std::shared_ptr<ports::ISession> s, const std::error_code& ec)
{
    Logger::info("[query] disconnect: " + s->remote_endpoint() +
                 (ec ? (" ec=" + ec.message()) : ""));
    peers_.erase(s.get());
}

// ----------------------------------------------------------------------------
// Byte-stream entry point. Appends bytes into the per-connection buffer,
// then repeatedly extracts complete frames: [u16 size][u16 msg_id][payload...]
// If the buffer would exceed max_buf_, it is cleared and the socket closed
// to avoid unbounded memory growth.
// バイトストリームの入口。接続ごとのバッファに追記し、
// [u16 size][u16 msg_id][payload...] 形式の完全フレームを繰り返し取り出す。
// バッファが max_buf_ を超える場合は、メモリ肥大化を防ぐためクリアして切断。
void QueryHandler::on_data(std::shared_ptr<ports::ISession> s, std::span<const std::uint8_t> bytes)
{
    auto it = peers_.find(s.get());
    if (it == peers_.end()) return;
    auto& c = it->second;

    if (c.buf.size() + bytes.size() > max_buf_)
    {
        Logger::warn("[query] buffer overflow: clearing and closing");
        c.buf.clear();
        s->close();
        return;
    }

    c.buf.insert(c.buf.end(), bytes.begin(), bytes.end());

    // Greedy frame extraction: parse as many complete frames as available.
    // 貪欲にフレーム抽出：取り出せるだけ完全フレームを処理する。
    for (;;)
    {
        if (c.buf.size() < 4)
            break;  // need at least size+msg_id
                    // 最低でも size+msg_id が必要

        const auto size = r16(c.buf.data());
        if (size < 4)
        {
            // Invalid header, drop accumulated bytes to resync.
            // 不正ヘッダー。蓄積バッファを破棄して再同期。
            c.buf.clear();
            break;
        }

        if (c.buf.size() < size)
            break;  // wait for more bytes
                    // まだ全体が来ていない

        // Cut one frame and advance the buffer.
        // フレームを切り出してバッファを前進。
        std::vector<std::uint8_t> fr(c.buf.begin(), c.buf.begin() + size);
        c.buf.erase(c.buf.begin(), c.buf.begin() + size);

        handle_frame(c, std::span<const std::uint8_t>(fr.data(), fr.size()));
    }
}

// ----------------------------------------------------------------------------
// Handle a single framed message. Decodes header, inspects msg_id,
// and routes known messages. Unknown ids are ignored silently.
// 1 つのフレームを処理。ヘッダーを読み取り、msg_id により分岐。
// 未知の ID は黙って無視。
void QueryHandler::handle_frame(Conn& c, std::span<const std::uint8_t> fr)
{
    if (fr.size() < 4) return;

    const auto size = r16(fr.data());
    const auto msg_id = r16(fr.data() + 2);
    const auto payload = fr.subspan(4, size - 4);

    if (msg_id == MSG_THANATOS_QUERY)
    {
        // Payload itself is a length-prefixed blob: [u16 n][n bytes].
        // ペイロード本体は長さ付きブロブ: [u16 n][n バイト]
        std::vector<std::uint8_t> blob;
        if (!decode_blob(payload, blob)) return;

        // Forward blob to the currently active RO client session (if any).
        // 現在アクティブな RO クライアントセッションへ転送（存在する場合）。
        auto cli = registry_->get_active_client();
        if (!cli)
        {
            Logger::info("[query] no active client to deliver GG blob");
            return;
        }

        // Register a one-shot waiter: the next C2S bytes seen by the registry
        // will be captured and replied back to this query peer as a framed
        // MSG_THANATOS_REPLY with a length-prefixed blob payload.
        // 1 回だけの応答待ちを登録：レジストリで次に観測される C2S バイト列を捕捉し、
        // 長さ付きブロブを MSG_THANATOS_REPLY としてこの相手に返送する。
        registry_->wait_next_c2s([](std::span<const std::uint8_t>) { return true; },
                                 [sess = c.session](std::span<const std::uint8_t> reply_bytes)
                                 {
                                     auto pld = encode_blob(reply_bytes);
                                     auto out = frame(MSG_THANATOS_REPLY, pld);
                                     sess->send(out);
                                 });

        // Kick the blob toward the client now.
        // ブロブをクライアント側へ送出。
        cli->send(std::span<const std::uint8_t>(blob.data(), blob.size()));
    }
    else
    {
        // Unknown message id: intentionally ignored to keep wire forward-compatible.
        // 未知のメッセージ ID：将来互換のため意図的に無視。
    }
}

}  // namespace arkan::thanatos::interface::query
