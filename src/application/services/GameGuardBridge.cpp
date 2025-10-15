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

// Randomize the timeout
std::chrono::milliseconds GameGuardBridge::get_randomized_timeout()
{
    if (!randomize_timeout_)
    {
        return base_timeout_;
    }

    // Generates a random value between min_percent% and max_percent% of the base timeout
    std::uniform_int_distribution<int> dist(timeout_min_percent_, timeout_max_percent_);
    int percent = dist(rng_);

    auto randomized = std::chrono::milliseconds(base_timeout_.count() * percent / 100);

    Logger::debug("[gg] randomized timeout: " + std::to_string(randomized.count()) + "ms (" +
                  std::to_string(percent) + "% of base " + std::to_string(base_timeout_.count()) +
                  "ms)");

    return randomized;
}

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
        Logger::debug("[gg] drop query: no client wire");
        return;
    }

    // Resets the retry counter for a new query
    retry_count_ = 0;
    pending_ = true;

    // Uses randomized timeout
    timeout_ = get_randomized_timeout();
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
            gg_query.resize(pkt_len);
            Logger::debug("[gg] note: coalesced client frame, trimming to pkt_len");
        }
    }
    else
    {
        pkt_len = static_cast<uint16_t>(4 + gg_query.size());
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

    // Saves the package for possible retry
    last_gg_query_ = gg_query;

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

    Logger::debug(banner("TX→CLIENT", "GG 09CF", gg_query.size()));
    Logger::debug(ro_header(gg_query.data(), gg_query.size()));
    Logger::debug("\n" + hex_dump(gg_query.data(), gg_query.size()));
}

bool GameGuardBridge::maybe_consume_c2s(const std::uint8_t* data, std::size_t len)
{
    if (!pending_)
    {
        if (len >= 2 && rd16le(data) == 0x09D0)
        {
            Logger::debug("[gg] drop late 09D0 while pending=false");
            return true;
        }
        return false;
    }

    // automatic retry
    if (Clock::now() > deadline_)
    {
        if (retry_count_ < max_retries_)
        {
            retry_count_++;

            // Randomize the timeout again for each retry
            timeout_ = get_randomized_timeout();
            deadline_ = Clock::now() + timeout_;

            Logger::debug("[gg] timeout waiting client GG reply - retry attempt " +
                          std::to_string(retry_count_) + "/" + std::to_string(max_retries_));

            // Resends the same packet
            if (wire_ && !last_gg_query_.empty())
            {
                if (wire_->send_to_client(last_gg_query_))
                {
                    Logger::debug("[gg] retry: resent 09CF len=" +
                                  std::to_string(last_gg_query_.size()));

                    Logger::debug(banner("TX→CLIENT (RETRY)", "GG 09CF", last_gg_query_.size()));
                    Logger::debug(ro_header(last_gg_query_.data(), last_gg_query_.size()));
                    Logger::debug("\n" + hex_dump(last_gg_query_.data(), last_gg_query_.size()));
                }
                else
                {
                    Logger::error("[gg] retry: failed to resend 09CF");
                    pending_ = false;
                    retry_count_ = 0;
                    last_gg_query_.clear();
                }
            }
            else
            {
                Logger::error("[gg] retry: no wire or query to resend");
                pending_ = false;
                retry_count_ = 0;
                last_gg_query_.clear();
            }

            return false;
        }
        else
        {
            // Exhausted all attempts
            Logger::error("[gg] timeout after " + std::to_string(max_retries_) +
                          " retries - giving up");
            pending_ = false;
            retry_count_ = 0;
            last_gg_query_.clear();
            return false;
        }
    }

    if (len < 2) return false;

    const uint16_t op = rd16le(data);

    // Accept 0x09D0 (primary) and 0x099F (some older clients). Others are unrelated.
    // 0x09D0（主）と 0x099F（古いクライアント）を許可。他は無関係。
    if (op == 0x09D0 || op == 0x099F)
    {
        if (len < 4) return false;

        std::size_t eff_len = trim_to_pkt_len(data, len);

        std::vector<std::uint8_t> reply;
        reply.reserve(eff_len);
        reply.assign(data, data + eff_len);

        if (retry_count_ > 0)
        {
            Logger::debug("[gg] SUCCESS after " + std::to_string(retry_count_) +
                          " retries - received " + (op == 0x09D0 ? "09D0" : "099F"));
        }

        Logger::debug("[gg] strategy=FULL_FRAME send head16=" +
                      arkan::thanatos::shared::hex::hex(reply.data(),
                                                        std::min<std::size_t>(reply.size(), 16)) +
                      " len=" + std::to_string(reply.size()));

        Logger::debug(banner("RX←CLIENT", (op == 0x09D0 ? "GG 09D0" : "GG 099F"), reply.size()));
        Logger::debug(ro_header(reply.data(), reply.size()));
        Logger::debug("\n" + hex_dump(reply.data(), reply.size()));

        query_.sendReply(reply);

        pending_ = false;
        retry_count_ = 0;
        last_gg_query_.clear();

        Logger::debug("[gg] state: pending=false (GG reply delivered; consumed)");
        return true;
    }

    return false;
}

}  // namespace arkan::thanatos::application::services