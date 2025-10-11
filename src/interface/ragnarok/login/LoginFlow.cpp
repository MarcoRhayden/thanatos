#include "LoginFlow.hpp"

#include <string>

#include "interface/ragnarok/dto/LoginDTO.hpp"
#include "interface/ragnarok/mappers/LoginMapper.hpp"
#include "interface/ragnarok/model/PhaseSignal.hpp"

namespace arkan::thanatos::interface::ro::loginflow
{

// =========================================================================
// Aliases for clean code / 可読性のための別名
// =========================================================================
namespace dto = arkan::thanatos::interface::ro::dto;
namespace mappers = arkan::thanatos::interface::ro::mappers;
// =========================================================================

LoginFlow::LoginFlow(LoginCfg& cfg, LoginState& st, SendFn send, LogFn log)
    : cfg_(cfg), st_(st), send_(std::move(send)), log_(std::move(log))
{
    // ctor keeps references only; ownership stays at the caller.
    // 参照のみ保持。所有権は呼び出し側に残る。
}

void LoginFlow::handle(uint16_t opcode, const uint8_t* /*data*/, size_t /*len*/)
{
    // Dispatch by opcode to keep the login state machine explicit and testable.
    // オペコードで分岐し、ログイン状態遷移を明確かつテストしやすくする。
    switch (opcode)
    {
        // ===== secure handshake =====
        // Client wants the server’s secure key preamble before token/login.
        // トークン/ログイン前のセキュアキー要求。
        case 0x01DB:
        case 0x0204:
            onSecureHandshake();
            return;

        // ===== token =====
        // Client asks for a login token right after secure handshake.
        // セキュアハンドシェイク直後にトークン要求。
        case 0x0ACF:
        case 0x0C26:
            onTokenRequest();
            return;

        // ===== master login (several variants) =====
        // Different client builds send different master-login opcodes.
        // クライアントビルドにより異なるマスター・ログイン・オペコードを使用。
        case 0x0064:
        case 0x01DD:
        case 0x01FA:
        case 0x0AAC:
        case 0x0B04:
        case 0x0987:
        case 0x0A76:
        case 0x2085:
        case 0x2B0D:
        case 0x1DD5:
        case 0x0825:
            onMasterLogin(opcode);
            return;

        default:
            // Unknown/unsupported opcode: keep running but log for diagnostics.
            // 未知/未対応のオペコード。動作継続しつつ診断ログを残す。
            log_(std::string("unhandled opcode=0x") + std::to_string(opcode));
    }
}

void LoginFlow::onSecureHandshake()
{
    // Minimal server reply: send a fixed-format secure key packet.
    // 最小限の応答：固定形式のセキュアキー・パケットを送信。
    log_("secure login handshake request");

    // DTO -> mapper -> legacy packet bytes (keeps wire-compat).
    // DTOからマッパー経由でレガシーパケットへ（線路互換性を維持）。
    send_(mappers::to_packet(dto::SecureLoginKeyInfo{}));
}

void LoginFlow::onTokenRequest()
{
    // After secure handshake, most clients expect a token payload.
    // セキュアハンドシェイク後、クライアントはトークン受領を期待。
    log_("token request");

    // Use default token format through the mapper to match legacy bytes.
    // マッパーで既存のバイト列と一致させるため、デフォルトトークン形式を使用。
    send_(mappers::to_packet(dto::LoginTokenInfo{}));

    // Gate master login: ignore master-login opcodes until token step is done.
    // マスターログインを受け付けるゲート。トークン処理完了までは無視する。
    awaiting_master_ = true;
}

void LoginFlow::onMasterLogin(uint16_t opcode)
{
    // Protect against out-of-order or duplicate master-login attempts.
    // 順序違反や重複ログイン要求から保護。
    if (!awaiting_master_)
    {
        log_("master login received but not awaiting; ignoring");
        return;
    }
    awaiting_master_ = false;
    st_.lastMasterOpcode = opcode;

    // Fixed IDs used to emulate a successful auth session.
    // 認証成功セッションをエミュレートする固定ID。
    st_.accountID = {0x81, 0x84, 0x1E, 0x00};
    st_.sessionID = {0x00, 0x5E, 0xD0, 0xB2};
    st_.sessionID2 = {0xFF, 0x00, 0x00, 0x00};

    log_("master login (auto) opcode=0x" + std::to_string(opcode) + " -> account_server_info");

    // Build a single DTO that aggregates all server-list info.
    // サーバー一覧情報を1つのDTOに集約。
    dto::AccountServerInfo server_info_dto;
    server_info_dto.session_id = st_.sessionID;  // must match token step
    // トークンと整合するセッションID
    server_info_dto.account_id = st_.accountID;  // displayed at char server
    // キャラクタサーバ側で参照されるアカウントID
    server_info_dto.session_id2 = st_.sessionID2;  // cookie-like extra
    // 追加クッキー相当データ
    server_info_dto.host_ip = cfg_.hostIp;  // IP bytes in network order
    // ネットワーク順のIPバイト列
    server_info_dto.host_port = cfg_.hostPortLE;  // keep LE, mapper writes it as legacy did
    // LE保持。レガシーと同じ書き方で送る（バイトスワップ禁止）

    server_info_dto.server_name = cfg_.serverName;  // appears on server list UI
    // サーバー一覧UIに表示される名称
    server_info_dto.users_online = cfg_.usersOnline;  // cosmetic only
    // 見た目上のオンライン人数
    server_info_dto.is_male = cfg_.male;  // sex flag used by some clients
    // 一部クライアントが参照する性別フラグ

    // Some opcodes require modern layout (0x0AC4) instead of classic (0x0069).
    // 一部のオペコードではクラシック(0x0069)ではなくモダン(0x0AC4)が必要。
    const bool needs_0AC4 =
        (opcode == 0x0825 || opcode == 0x2085 || opcode == 0x2B0D || opcode == 0x1DD5);

    if (needs_0AC4 || !cfg_.prefer0069)
    {
        server_info_dto.use_modern_format = true;  // send 0x0AC4
        // 0x0AC4 を送信
        log_("-> will send 0x0AC4 (new format)");
    }
    else
    {
        server_info_dto.use_modern_format = false;  // send 0x0069
        // 0x0069 を送信
        log_("-> will send 0x0069 (classic)");
    }

    // Serialize via mapper to preserve legacy byte-for-byte compatibility.
    // マッパーで直列化し、従来のバイト列互換を維持。
    send_(mappers::to_packet(server_info_dto));

    // Tell the process that the next TCP connect should go to Char server.
    // 次のTCP接続はキャラクタサーバへ向かうべきことを伝える。
    arkan::thanatos::interface::ro::g_expect_char_on_next_connect.store(true);
    log_("signaled g_expect_char_on_next_connect = true");
}

}  // namespace arkan::thanatos::interface::ro::loginflow
