#include "LoginFlow.hpp"

#include <atomic>
#include <random>
#include <string>

#include "interface/ragnarok/dto/LoginDTO.hpp"
#include "interface/ragnarok/mappers/LoginMapper.hpp"
#include "interface/ragnarok/model/PhaseSignal.hpp"
#include "interface/ragnarok/protocol/Ids.hpp"
#include "interface/ragnarok/protocol/Opcodes.hpp"

namespace arkan::thanatos::interface::ro::loginflow
{

// =========================================================================
// Aliases for clean code / 可読性のための別名
// =========================================================================
namespace dto = arkan::thanatos::interface::ro::dto;
namespace mappers = arkan::thanatos::interface::ro::mappers;
using arkan::thanatos::interface::ro::protocol::ClientPacket;
using arkan::thanatos::interface::ro::protocol::opcode_u16;
using arkan::thanatos::interface::ro::protocol::to_le_bytes;

// =========================================================================
// ID utilities / 連番・乱数ユーティリティ
// =========================================================================
namespace
{
// Global atomic counter for unique account IDs (per-process monotonic).
// アカウントID用の単純な単調増加カウンタ（プロセス内で一意）。
std::atomic<std::uint32_t> g_next_account_id{2000001};

// Thread-local RNG for session IDs (not security-critical; just uniqueness).
// セッションID用のスレッドローカル乱数（セキュリティではなく一意性目的）。
std::uint32_t generate_session_id()
{
    static thread_local std::random_device rd;
    static thread_local std::mt19937 gen(rd());
    static thread_local std::uniform_int_distribution<std::uint32_t> dis(1000000000u, 4000000000u);
    return dis(gen);
}

bool requiresModernAccountServerInfo(std::uint16_t opcode)
{
    return opcode == opcode_u16(ClientPacket::MasterLogin0825) ||
           opcode == opcode_u16(ClientPacket::MasterLogin2085) ||
           opcode == opcode_u16(ClientPacket::MasterLogin2B0D) ||
           opcode == opcode_u16(ClientPacket::MasterLogin1DD5);
}
}  // namespace

/* -----------------------------------------------------------------------------
   handle
   - Opcode-driven dispatcher for the login handshake.
   - ログインハンドシェイク用のオペコードディスパッチャ。
----------------------------------------------------------------------------- */
void LoginFlow::handle(std::uint16_t opcode, const std::uint8_t* /*data*/, std::size_t /*len*/)
{
    switch (opcode)
    {
        // ===== secure handshake =====
        // client → asks for server secure key preamble.
        // クライアントがセキュア鍵の前置を要求。
        case opcode_u16(ClientPacket::SecureLoginRequest):
        case opcode_u16(ClientPacket::SecureLoginRequestAlt):
            onSecureHandshake();
            return;

        // ===== token =====
        // client → asks for a login token right after secure handshake.
        // セキュアハンドシェイク直後のトークン要求。
        case opcode_u16(ClientPacket::LoginTokenRequest):
        case opcode_u16(ClientPacket::LoginTokenRequestAlt):
            onTokenRequest();
            return;

        // ===== master login (several variants) =====
        // Different client builds emit different master-login opcodes.
        // クライアントビルドによりマスターログインのオペコードが異なる。
        case opcode_u16(ClientPacket::MasterLoginClassic):
        case opcode_u16(ClientPacket::MasterLogin01DD):
        case opcode_u16(ClientPacket::MasterLogin01FA):
        case opcode_u16(ClientPacket::MasterLogin0AAC):
        case opcode_u16(ClientPacket::MasterLogin0B04):
        case opcode_u16(ClientPacket::MasterLogin0987):
        case opcode_u16(ClientPacket::MasterLogin0A76):
        case opcode_u16(ClientPacket::MasterLogin2085):
        case opcode_u16(ClientPacket::MasterLogin2B0D):
        case opcode_u16(ClientPacket::MasterLogin1DD5):
        case opcode_u16(ClientPacket::MasterLogin0825):
            onMasterLogin(opcode);
            return;

        default:
            // Unknown/unsupported → keep running, but log for diagnostics.
            // 未知/未対応はログに残して継続。
            log_(std::string("unhandled opcode=0x") + std::to_string(opcode));
    }
}

/* -----------------------------------------------------------------------------
   onSecureHandshake → 0x01DC
   - Minimal fixed reply (mapper builds legacy-correct bytes).
   - 固定応答（マッパーが従来どおりのバイト列に整形）。
----------------------------------------------------------------------------- */
void LoginFlow::onSecureHandshake()
{
    log_("secure login handshake request");
    send_(mappers::to_packet(dto::SecureLoginKeyInfo{}));
}

/* -----------------------------------------------------------------------------
   onTokenRequest → 0x0AE3
   - Generate a fresh token via the injected ITokenGenerator.
   - 依存注入された ITokenGenerator でトークンを生成して返信。
----------------------------------------------------------------------------- */
void LoginFlow::onTokenRequest()
{
    log_("token request");

    // EN: Produce a short, URL-safe token (e.g., Base64URL-encoded 12 random bytes -> 16 chars).
    // JP: URL セーフで短いトークンを生成（例：12バイト乱数→Base64URL 16文字）。
    dto::LoginTokenInfo tok;
    tok.token = tokenGen_.makeLoginToken();

    send_(mappers::to_packet(tok));

    // Gate: accept master-login only after token step.
    // ゲート：トークン送信後のみマスターログインを受け付ける。
    awaiting_master_ = true;

    log_("issued login token (len=" + std::to_string(tok.token.size()) + ")");
}

/* -----------------------------------------------------------------------------
   onMasterLogin → 0x0069 or 0x0AC4
   - Finalizes server-list info and signals "expect char connect".
   - サーバ一覧情報を確定し、「次は Char 接続」をシグナル。
----------------------------------------------------------------------------- */
void LoginFlow::onMasterLogin(std::uint16_t opcode)
{
    if (!awaiting_master_)
    {
        log_("master login received but not awaiting; ignoring");
        return;
    }
    awaiting_master_ = false;
    st_.lastMasterOpcode = opcode;

    // Generate unique account/session IDs per connection.
    // 接続毎にアカウント/セッションIDを生成。
    const std::uint32_t account_id_num = g_next_account_id.fetch_add(1, std::memory_order_relaxed);
    st_.accountID = to_le_bytes(account_id_num);
    st_.sessionID = to_le_bytes(generate_session_id());
    st_.sessionID2 = to_le_bytes(generate_session_id());

    log_("master login opcode=0x" + std::to_string(opcode) +
         " -> account_id=" + std::to_string(account_id_num));

    // Aggregate all server-list fields in a single DTO.
    // サーバ一覧用の各項目を1つのDTOへ集約。
    dto::AccountServerInfo server_info_dto;
    server_info_dto.session_id = st_.sessionID;
    server_info_dto.account_id = st_.accountID;
    server_info_dto.session_id2 = st_.sessionID2;
    server_info_dto.host_ip = cfg_.hostIp;        // network-order bytes
    server_info_dto.host_port = cfg_.hostPortLE;  // mapper writes LE as in legacy
    server_info_dto.server_name = cfg_.serverName;
    server_info_dto.users_online = cfg_.usersOnline;
    server_info_dto.is_male = cfg_.male;

    // Some master opcodes imply the modern account-server packet (0x0AC4).
    // 一部のマスターオペコードではモダン形式(0x0AC4)が必要。
    const bool needs_0AC4 = requiresModernAccountServerInfo(opcode);

    if (needs_0AC4 || !cfg_.prefer0069)
    {
        server_info_dto.use_modern_format = true;  // send 0x0AC4
        log_("-> will send 0x0AC4 (new format)");
    }
    else
    {
        server_info_dto.use_modern_format = false;  // send 0x0069
        log_("-> will send 0x0069 (classic)");
    }

    // Serialize via mapper (byte-for-byte wire compatibility with legacy).
    // マッパー経由で直列化（レガシーのバイト列と完全互換）。
    send_(mappers::to_packet(server_info_dto));

    // Signal that the next TCP connect should target the Char server.
    // 次の TCP 接続先は Char サーバであると通知。
    arkan::thanatos::interface::ro::g_expect_char_on_next_connect.store(true);
    log_("signaled g_expect_char_on_next_connect = true");
}

}  // namespace arkan::thanatos::interface::ro::loginflow
