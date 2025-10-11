#include "application/services/GameGuardBridge.hpp"

#include <string>

#include "infrastructure/log/Logger.hpp"
#include "interface/ragnarok/protocol/Codec.hpp"
#include "shared/Hex.hpp"

using arkan::thanatos::infrastructure::log::Logger;
using namespace arkan::thanatos::interface::ro::protocol;

namespace arkan::thanatos::application::services
{

namespace
{
// Poseidon (Perl) hotfix truncates the GG frame to 18 bytes.
// ポセイドン(Perl)のホットフィックスは GG フレーム全体を 18 バイトに切り詰める。
constexpr std::size_t GG_TRUNCATED_LEN = 18;

// Utility: clamp to declared packet length if present.
// パケット先頭の length に合わせて長さを丸める補助関数。
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
    wire_ = wire;
    Logger::debug("[gg] bound client wire");
}

// -----------------------------------------------------------------------------
// Forward 09CF to the RO client as-is (frame 09CF|len|payload).
// 09CF はクライアントへそのまま転送（09CF|長さ|ペイロード）。
// -----------------------------------------------------------------------------
void GameGuardBridge::on_query_from_kore_(std::vector<std::uint8_t> gg_query)
{
    if (pending_)
    {
        Logger::debug("[gg] drop query: already pending");
        return;
    }
    if (!wire_)
    {
        Logger::debug("[gg] drop query: no client wire");
        return;
    }

    pending_ = true;
    deadline_ = Clock::now() + timeout_;
    sent_at_ = Clock::now();

    // If query is already a framed 09CF, trim to its internal length.
    // 既に 09CF でフレーム化されている場合は length に合わせて丸める。
    const bool already_09CF = gg_query.size() >= 4 && rd16le(gg_query.data()) == 0x09CF;
    uint16_t pkt_len = 0;
    if (already_09CF)
    {
        pkt_len = rd16le(gg_query.data() + 2);
        if (pkt_len >= 4 && pkt_len <= gg_query.size() && pkt_len != gg_query.size())
        {
            gg_query.resize(pkt_len);
            Logger::debug("[gg] note: coalesced client frame, trimming to pkt_len");
        }
    }
    else
    {
        pkt_len = static_cast<uint16_t>(4 + gg_query.size());
        // Frame it as 09CF|len|payload.
        // 09CF|長さ|ペイロード の形にフレーム化。
        std::vector<std::uint8_t> framed;
        framed.reserve(pkt_len);
        wr16le(framed, 0x09CF);
        wr16le(framed, pkt_len);
        framed.insert(framed.end(), gg_query.begin(), gg_query.end());
        gg_query.swap(framed);
    }

    // Remember the 09CF total length from header (for AUTO strategy).
    // 直前の 09CF 総バイト数を記録（AUTO 戦略用）。
    last_gg_request_len_ = pkt_len;  // e.g., 72 or 80

    if (!wire_->send_to_client(gg_query))
    {
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
}

// -----------------------------------------------------------------------------
// Intercept client->server and, only for 0x09D0 (or 0x099F), build Poseidon reply
// according to the selected strategy. Keepalives (0x0360) are ignored.
// 0x09D0（必要なら 0x099F）だけを処理し、選択された戦略で Poseidon 返信を構築。
// 0x0360 などのキープアライブは無視。
// -----------------------------------------------------------------------------
bool GameGuardBridge::maybe_consume_c2s(const std::uint8_t* data, std::size_t len)
{
    // Late 09D0 after window → drop to avoid double-processing.
    // ペンディング外の 09D0 は破棄（重複処理を避ける）。
    if (!pending_)
    {
        if (len >= 2 && rd16le(data) == 0x09D0)
        {
            Logger::debug("[gg] drop late 09D0 while pending=false");
            return true;  // consume late duplicates
        }
        return false;
    }

    // Timeout handling.
    // タイムアウト処理。
    if (Clock::now() > deadline_)
    {
        pending_ = false;
        Logger::debug("[gg] timeout waiting client GG reply");
        return false;
    }

    if (len < 2) return false;

    const uint16_t op = rd16le(data);

    // Accept only 0x09D0 (optionally 0x099F). Ignore everything else.
    // 0x09D0（必要なら 0x099F）のみ処理。他は無視。
    if (op == 0x09D0 || op == 0x099F)
    {
        if (len < 4) return false;

        // Respect internal packet length (LE at +2).
        // パケット内部の length（+2, LE）を尊重。
        std::size_t eff_len = trim_to_pkt_len(data, len);

        std::vector<std::uint8_t> reply;
        reply.reserve(eff_len);

        // AUTO strategy selection by last 09CF length.
        // AUTO 戦略：直前の 09CF の長さで決定。
        GGStrategy s = strategy_;
        if (s == GGStrategy::AUTO)
        {
            if (last_gg_request_len_ == 72)
                s = GGStrategy::BODY_TRUNC_18;
            else if (last_gg_request_len_ == 80)
                s = GGStrategy::FULL_FRAME;
            else
                s = GGStrategy::FULL_FRAME;  // fallback
            Logger::debug("[gg] AUTO chosen: last 09CF=" + std::to_string(last_gg_request_len_) +
                          " -> " +
                          (s == GGStrategy::BODY_TRUNC_18 ? "BODY_TRUNC_18"
                           : s == GGStrategy::FULL_FRAME  ? "FULL_FRAME"
                                                          : "BODY_LEN"));
        }

        // --- Strategies ------------------------------------------------------
        switch (s)
        {
            case GGStrategy::FULL_FRAME:
            {
                // Send full 09D0 frame (Perl RagnarokServer.pm behavior).
                // 09D0 フレームを丸ごと返す（Perl RagnarokServer.pm と同様）。
                reply.assign(data, data + eff_len);
                Logger::debug("[gg] strategy=FULL_FRAME send head32=" +
                              arkan::thanatos::shared::hex::hex(
                                  reply.data(), std::min<std::size_t>(reply.size(), 16)) +
                              " len=" + std::to_string(reply.size()));
                break;
            }
            case GGStrategy::BODY_TRUNC_18:
            {
                // Send the FIRST 18 BYTES of the FULL 09D0 frame (including opcode+len).
                // フレーム全体の先頭18バイト（opcode+lenを含む）を送る。Perl EmbedServer.pm
                // と同じ。
                const std::size_t out_len = std::min<std::size_t>(GG_TRUNCATED_LEN, eff_len);
                reply.assign(data, data + out_len);
                Logger::debug("[gg] strategy=BODY_TRUNC_18 send head32=" +
                              arkan::thanatos::shared::hex::hex(
                                  reply.data(), std::min<std::size_t>(reply.size(), 16)) +
                              " len=" + std::to_string(reply.size()));
                break;
            }
            case GGStrategy::BODY_LEN:
            {
                // Send only body using internal length (diagnostic).
                // 本体のみを内部長で返す（診断用）。
                const std::size_t body_len = eff_len - 4;
                reply.assign(data + 4, data + 4 + body_len);
                Logger::debug("[gg] strategy=BODY_LEN send bodyHead=" +
                              arkan::thanatos::shared::hex::hex(
                                  reply.data(), std::min<std::size_t>(reply.size(), 16)) +
                              " bodyLen=" + std::to_string(reply.size()));
                break;
            }
            case GGStrategy::AUTO:
            {
                // handled above / 上で決定済み
                break;
            }
        }

        // Deliver to Kore as Poseidon Reply.
        // Kore 側へ Poseidon 返信として配送。
        query_.sendReply(reply);

        // Consume 09D0 so it won't leak into Char/Map stubs.
        // ここで消費し、Char/Map 側へ流さない。
        pending_ = false;
        Logger::debug("[gg] state: pending=false (GG reply delivered; consumed)");
        return true;  // CONSUME
    }

    // Ignore everything else (e.g., 0x0360 keepalive).
    // その他（例：0x0360 キープアライブ）は無視。
    return false;
}

}  // namespace arkan::thanatos::application::services
