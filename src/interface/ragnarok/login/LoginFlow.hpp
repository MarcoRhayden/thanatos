#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "application/ports/crypto/ITokenGenerator.hpp"
#include "interface/ragnarok/protocol/Codec.hpp"

namespace arkan
{
namespace thanatos
{
namespace interface
{
namespace ro
{
namespace loginflow
{

using arkan::thanatos::interface::ro::protocol::Packet;
using TokenGenerator = arkan::thanatos::application::ports::crypto::ITokenGenerator;

/* -----------------------------------------------------------------------------
   LoginCfg
   - Static knobs used to craft server-list/account-server packets.
   - サーバ一覧/アカウントサーバのパケット生成に使う静的設定。
----------------------------------------------------------------------------- */
struct LoginCfg
{
    std::array<std::uint8_t, 4> hostIp{127, 0, 0, 1};
    std::uint16_t hostPortLE{6900};  // keep LE: mapper writes as legacy did
    std::string serverName{"Arkan Software"};
    std::uint32_t usersOnline{77};
    bool male{true};
    bool prefer0069{true};  // if false (or opcode needs), send 0x0AC4
};

/* -----------------------------------------------------------------------------
   LoginState
   - Per-connection mutable state (IDs/opcode, etc.).
   - 接続毎の可変状態（IDやオペコードなど）。
----------------------------------------------------------------------------- */
struct LoginState
{
    // IDs now generated dynamically in LoginFlow::onMasterLogin().
    // いまは onMasterLogin() で動的生成。
    std::array<std::uint8_t, 4> accountID{};
    std::array<std::uint8_t, 4> sessionID{};
    std::array<std::uint8_t, 4> sessionID2{};
    std::uint16_t lastMasterOpcode{0};
};

/* -----------------------------------------------------------------------------
   LoginFlow
   - Handles the login handshake: secure key -> token -> master login.
   - ログインの一連処理を担当：セキュア鍵 -> トークン -> マスターログイン。
   - Token is produced by an injected ITokenGenerator (DI for testability).
   - トークンは注入された ITokenGenerator で生成（テスタビリティ向上）。
----------------------------------------------------------------------------- */
class LoginFlow
{
   public:
    using SendFn = std::function<void(const Packet&)>;
    using LogFn = std::function<void(const std::string&)>;

    LoginFlow(LoginCfg& cfg, LoginState& st, SendFn send, LogFn log,
              TokenGenerator& tokenGen)  // inject token generator (DI)
        : cfg_(cfg), st_(st), send_(std::move(send)), log_(std::move(log)), tokenGen_(tokenGen)
    {
    }

    // Single-event entrypoint: decode 'opcode' and dispatch.
    // 入力は1イベント：オペコードで分岐。
    void handle(std::uint16_t opcode, const std::uint8_t* data, std::size_t len);

   private:
    void onSecureHandshake();                  // responds with 0x01DC
    void onTokenRequest();                     // responds with 0x0AE3 (uses tokenGen_)
    void onMasterLogin(std::uint16_t opcode);  // responds with 0x0069 or 0x0AC4

   private:
    LoginCfg& cfg_;
    LoginState& st_;
    SendFn send_;
    LogFn log_;
    TokenGenerator& tokenGen_;      // source of truth for login token
                                    // トークン生成の実体（依存注入）
    bool awaiting_master_ = false;  // gate to ignore premature master-login
};

}  // namespace loginflow
}  // namespace ro
}  // namespace interface
}  // namespace thanatos
}  // namespace arkan
