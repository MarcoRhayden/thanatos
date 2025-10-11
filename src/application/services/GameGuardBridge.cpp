#include "application/services/GameGuardBridge.hpp"

#include <string>

#include "infrastructure/log/Logger.hpp"
#include "interface/ragnarok/protocol/Codec.hpp"
#include "shared/Hex.hpp"
#include "shared/LogFmt.hpp"

using arkan::thanatos::infrastructure::log::Logger;
using arkan::thanatos::shared::logfmt::banner;
using arkan::thanatos::shared::logfmt::hex_dump;
using arkan::thanatos::shared::logfmt::ro_header;
using namespace arkan::thanatos::interface::ro::protocol;

namespace arkan::thanatos::application::services
{

namespace
{
/*
 * Trim inbound bytes to the packet’s own length field if present.
 * - RO frames are [u16 opcode][u16 length][payload…] in little-endian.
 * - When GUIs or proxies coalesce multiple frames, we may receive extra bytes; trimming ensures
 *   we forward exactly one logical frame.
 * 先頭の length に合わせてバッファ長を丸める。
 * - ROフレームは LE の [u16 opcode][u16 length][payload…] 構造。
 * - GUI/プロキシが複数フレームを連結する場合があるため、実際の1フレーム分に切り詰める。
 */
inline std::size_t trim_to_pkt_len(const std::uint8_t* data, std::size_t L)
{
    if (L >= 4)
    {
        const std::uint16_t pkt_len = rd16le(data + 2);
        if (pkt_len >= 4 && pkt_len <= L) return static_cast<std::size_t>(pkt_len);
    }
    return L;
}
}  // namespace

GameGuardBridge::GameGuardBridge(ports::query::IQueryServer& query) : query_(query)
{
    /*
     * The bridge subscribes to Poseidon "Query" messages.
     * - Kore (via Poseidon) sends the raw GG blob to us.
     * - We forward it to the live client (as 0x09CF) and later return the client’s reply
     *   back to Poseidon ("Reply").
     * ブリッジは Poseidon の「Query」に購読。
     * - Kore(→Poseidon) が GG 生データを送る。
     * - それをクライアントへ 0x09CF で送信し、クライアントの応答を「Reply」で返す。
     */
    Logger::debug("[gg] bridge constructed; wiring onQuery()");
    query_.onQuery(
        [this](std::vector<std::uint8_t> gg_query)
        {
            Logger::debug(std::string("[gg] kore->bridge query len=") +
                          std::to_string(gg_query.size()) +
                          (pending_ ? " (pending=true)" : " (pending=false)"));
            on_query_from_kore_(std::move(gg_query));
        });
}

void GameGuardBridge::bindClientWire(ports::net::IClientWire* wire)
{
    /*
     * The wire is an abstraction over the actual RO client socket.
     * We keep a raw pointer because lifetime is owned elsewhere; call sites must ensure validity.
     * wire は実クライアントソケットの抽象。所有権は外部にあり、生存期間の管理は呼び出し側。
     */
    wire_ = wire;
    Logger::debug("[gg] bound client wire");
}

// -----------------------------------------------------------------------------
// Forward 0x09CF to the RO client as-is (0x09CF | len | payload).
// 0x09CF をクライアントへそのまま転送（0x09CF | 長さ | ペイロード）。
// -----------------------------------------------------------------------------
void GameGuardBridge::on_query_from_kore_(std::vector<std::uint8_t> gg_query)
{
    // Only one outstanding GG transaction at a time; drop re-entrancy.
    // GG 取引は同時に一つだけ。再入は破棄。
    if (pending_)
    {
        Logger::debug("[gg] drop query: already pending");
        return;
    }
    if (!wire_)
    {
        // No active client socket; nothing we can forward to.
        // アクティブなクライアントがないため転送不可。
        Logger::debug("[gg] drop query: no client wire");
        return;
    }

    pending_ = true;
    deadline_ = Clock::now() + timeout_;
    sent_at_ = Clock::now();

    /*
     * Two input shapes are supported:
     *   Raw GG payload (no opcode/len) -> we wrap as 0x09CF.
     *   Already-framed 0x09CF|len|payload → we honor and (optionally) trim to its length.
     * 入力は2形態：
     *   素のGGペイロード -> 0x09CF でラップ
     *   既に 0x09CF フレーム -> length に合わせて尊重（必要なら切り詰め）
     */
    const bool already_09CF = gg_query.size() >= 4 && rd16le(gg_query.data()) == 0x09CF;
    uint16_t pkt_len = 0;
    if (already_09CF)
    {
        pkt_len = rd16le(gg_query.data() + 2);
        if (pkt_len >= 4 && pkt_len <= gg_query.size() && pkt_len != gg_query.size())
        {
            gg_query.resize(pkt_len);  // drop tail if the sender concatenated frames
            Logger::debug("[gg] note: coalesced client frame, trimming to pkt_len");
        }
    }
    else
    {
        pkt_len = static_cast<uint16_t>(4 + gg_query.size());
        // Build a well-formed 0x09CF frame.
        // 正当な 0x09CF フレームを構築。
        std::vector<std::uint8_t> framed;
        framed.reserve(pkt_len);
        wr16le(framed, 0x09CF);
        wr16le(framed, pkt_len);
        framed.insert(framed.end(), gg_query.begin(), gg_query.end());
        gg_query.swap(framed);
    }

    // Record the total 0x09CF size for AUTO strategies / analytics.
    // AUTO 戦略や解析用に 0x09CF 総サイズを記録。
    last_gg_request_len_ = pkt_len;

    if (!wire_->send_to_client(gg_query))
    {
        // Client socket failed; clear pending so Kore can retry.
        // クライアント送信失敗。pending を解除して Kore 側が再試行可能に。
        pending_ = false;
        Logger::debug("[gg] failed to send gg_query (09CF) to client; pending reset");
        return;
    }

    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(timeout_).count();
    const auto head8 = arkan::thanatos::shared::hex::hex(gg_query.data(),
                                                         std::min<std::size_t>(gg_query.size(), 8));

    Logger::debug(
        "[gg] bridge->client: forwarded GG query 09CF len=" + std::to_string(gg_query.size()) +
        " head8=" + head8 + " deadline=" + std::to_string(ms) + "ms");

    Logger::info(banner("TX→CLIENT", "GG 09CF", gg_query.size()));
    Logger::info(ro_header(gg_query.data(), gg_query.size()));
    Logger::info("\n" + hex_dump(gg_query.data(), gg_query.size()));
}

// -----------------------------------------------------------------------------
// Intercept client->server traffic and handle only 0x09D0 (or 0x099F) as GG reply.
// Keepalives (0x0360) and unrelated opcodes fall through.
// クライアント→サーバのうち、GG 応答は 0x09D0（場合により 0x099F）だけを処理。
// 0x0360 のようなキープアライブ等は素通し。
// -----------------------------------------------------------------------------
bool GameGuardBridge::maybe_consume_c2s(const std::uint8_t* data, std::size_t len)
{
    // If no GG request is in-flight, consume stray 0x09D0 packets to avoid confusing the stub.
    // 進行中の GG 要求が無ければ、迷子の 0x09D0 は飲み込んでスタブを混乱させない。
    if (!pending_)
    {
        if (len >= 2 && rd16le(data) == 0x09D0)
        {
            Logger::debug("[gg] drop late 09D0 while pending=false");
            return true;  // swallow late replies
        }
        return false;
    }

    // Timeout guard: if the client never replies, unlock the pipeline.
    // タイムアウト監視：クライアントが返さない場合はパイプラインを解放。
    if (Clock::now() > deadline_)
    {
        pending_ = false;
        Logger::debug("[gg] timeout waiting client GG reply");
        return false;
    }

    if (len < 2) return false;

    const uint16_t op = rd16le(data);

    // Accept 0x09D0 (primary) and 0x099F (some older clients). Others are unrelated.
    // 0x09D0（主）と 0x099F（古いクライアント）を許可。他は無関係。
    if (op == 0x09D0 || op == 0x099F)
    {
        if (len < 4) return false;

        // Respect internal length; some stacks may glue extra bytes behind the frame.
        // 内部の長さフィールドを尊重。背後に余分なバイトが付くケースに対応。
        std::size_t eff_len = trim_to_pkt_len(data, len);

        std::vector<std::uint8_t> reply;
        reply.reserve(eff_len);
        reply.assign(data, data + eff_len);

        Logger::debug("[gg] strategy=FULL_FRAME send head16=" +
                      arkan::thanatos::shared::hex::hex(reply.data(),
                                                        std::min<std::size_t>(reply.size(), 16)) +
                      " len=" + std::to_string(reply.size()));

        Logger::info(banner("RX←CLIENT", (op == 0x09D0 ? "GG 09D0" : "GG 099F"), reply.size()));
        Logger::info(ro_header(reply.data(), reply.size()));
        Logger::info("\n" + hex_dump(reply.data(), reply.size()));

        // Return the client’s GG reply to Poseidon as a "Poseidon Reply" frame.
        // クライアントの応答を Poseidon へ「Poseidon Reply」として返送。
        query_.sendReply(reply);

        // Consume the packet so it doesn’t reach Char/Map server stubs.
        // このパケットはここで消費し、Char/Map のスタブへ流さない。
        pending_ = false;
        Logger::debug("[gg] state: pending=false (GG reply delivered; consumed)");
        return true;
    }

    // Any other opcode is unrelated to GG; do not consume.
    // その他のオペコードは GG と無関係。消費しない。
    return false;
}

}  // namespace arkan::thanatos::application::services
