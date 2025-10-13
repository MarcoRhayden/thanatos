#include "interface/ragnarok/login/LoginHandler.hpp"

#include "interface/ragnarok/protocol/Codec.hpp"

namespace arkan
{
namespace thanatos
{
namespace interface
{
namespace ro
{

namespace ports_net = arkan::thanatos::application::ports::net;
using namespace arkan::thanatos::interface::ro::protocol;

/* -----------------------------------------------------------------------------
   LoginHandler
   - Owns the "Login Server" side of the Ragnarok handshake.
   - Accepts raw client packets, lets the guard/bridge (GG) consume what it needs,
     then forwards the rest to LoginFlow, which builds proper server responses.
   - ラグナロクのログインサーバ処理を担当。
   - クライアントからのパケットを受信し、ガード/ブリッジ(GG)による消費を挟み、
     残りを LoginFlow に渡してサーバ応答を生成する。
----------------------------------------------------------------------------- */

LoginHandler::LoginHandler(std::shared_ptr<SessionRegistry> /*registry*/, const apc::Config& cfg,
                           OnLogFn logger)
    : log_(std::move(logger))
{
    // Parse configured host into IPv4 bytes (used in server list packet).
    // 設定されたホスト名をIPv4バイト列へ解決（サーバ一覧パケットで使用）。
    boost::system::error_code ec;
    auto addr = boost::asio::ip::make_address_v4(cfg.ro_host, ec);
    if (!ec)
    {
        auto bytes = addr.to_bytes();
        cfg_.hostIp = {bytes[0], bytes[1], bytes[2], bytes[3]};
    }

    // Seed protocol-facing configuration for the login server response.
    // ログイン応答用のプロトコル設定を初期化。
    cfg_.hostPortLE = 0;
    cfg_.serverName = "Arkan Software";
    cfg_.usersOnline = 5293;
    cfg_.male = true;        // default gender bit for preview
    cfg_.prefer0069 = true;  // prefer classic 0x0069 server-list opcode

#if defined(_WIN32) && defined(THANATOS_USE_WINCNG)
    rng_ = std::make_unique<arkan::thanatos::infrastructure::crypto::WinCngRng>();
#else
    rng_ = std::make_unique<arkan::thanatos::infrastructure::crypto::PortableRng>();
#endif
    tokenGen_ =
        std::make_unique<arkan::thanatos::infrastructure::crypto::Random16TokenGenerator>(*rng_);

    // Create the flow state machine that knows how to reply to login opcodes.
    // ログイン系オペコードに応答するフローステートマシンを生成。
    flow_ = std::make_unique<loginflow::LoginFlow>(
        cfg_, st_,
        /* send: write assembled packet to the active socket */
        /* 送信用ラムダ: 組み立て済みパケットをアクティブソケットへ送信 */
        [this](const std::vector<uint8_t>& p) { sendBytes(p); },
        /* log: forward textual logs to the provided sink */
        /* ログ用ラムダ: テキストログを指定先へ転送 */
        [this](const std::string& m) { log(m); },
        /* token generator (DI) */
        *tokenGen_);
}

/* -----------------------------------------------------------------------------
   log
   - Small helper to call the injected logger if present.
   - 依存注入されたロガーがあれば呼び出す薄いラッパ。
----------------------------------------------------------------------------- */
void LoginHandler::log(const std::string& msg)
{
    if (log_) log_(msg);
}

/* -----------------------------------------------------------------------------
   sendBytes
   - Sends a finished packet to the current session (if any).
   - 現在のセッションが存在する場合、完成したパケットを送信する。
----------------------------------------------------------------------------- */
void LoginHandler::sendBytes(const std::vector<uint8_t>& p)
{
    if (auto s = cur_.lock()) s->send(std::span<const uint8_t>(p.data(), p.size()));
}

/* -----------------------------------------------------------------------------
   on_connect
   - Invoked when a client connects to the login server socket.
   - Optionally binds the guard/bridge to this session (anti-cheat/keepalive).
   - クライアントがログインサーバへ接続した際に呼ばれる。
   - 必要に応じてガード/ブリッジをこのセッションへバインドする。
----------------------------------------------------------------------------- */
void LoginHandler::on_connect(std::shared_ptr<ports_net::ISession> s)
{
    cur_ = s;
    log("connected: " + s->remote_endpoint());

    // Bind SessionWire to the guard/bridge (if present).
    // Guard/Bridge があれば SessionWire をバインド。
    if (gg_)
    {
        if (!wire_)
            wire_ = std::make_unique<wire::SessionWire>(cur_);
        else
            wire_->reset(cur_);
        gg_->bindClientWire(wire_.get());
    }
}

/* -----------------------------------------------------------------------------
   on_disconnect
   - Invoked when the client disconnects or the socket errors.
   - Unbinds guard/bridge and clears the current session pointer.
   - クライアント切断/ソケットエラー時に呼ばれる。
   - ガード/ブリッジを解除し、現在セッションをクリア。
----------------------------------------------------------------------------- */
void LoginHandler::on_disconnect(std::shared_ptr<ports_net::ISession> s, const std::error_code& ec)
{
    (void)s;
    (void)ec;
    cur_.reset();

    if (gg_) gg_->bindClientWire(nullptr);

    log("disconnected");
}

/* -----------------------------------------------------------------------------
   on_data
   - Entry point for each inbound login packet.
   - First let the guard/bridge consume it if it wants; otherwise read the opcode
     and forward only the payload (sans 2-byte opcode) to LoginFlow::handle.
   - 受信したログイン用パケットの入口。
   - まずガード/ブリッジに処理機会を与え、未消費なら2バイトのオペコードを読み取り、
     ペイロード部分のみを LoginFlow::handle へ渡す。
----------------------------------------------------------------------------- */
void LoginHandler::on_data(std::shared_ptr<ports_net::ISession> s,
                           std::span<const std::uint8_t> bytes)
{
    cur_ = s;

    // Give guard/bridge a chance to fully consume the packet (e.g., keepalive).
    // ガード/ブリッジがパケットを丸ごと消費する可能性がある（例: キープアライブ）。
    if (gg_ && gg_->maybe_consume_c2s(bytes.data(), bytes.size()))
    {
        return;
    }

    // All valid packets must start with a 2-byte opcode.
    // 正常なパケットは2バイトのオペコードを持つ。
    if (bytes.size() < 2) return;

    const uint16_t op = rd16le(bytes.data());
    log(std::string("[login] rx opcode=0x") + arkan::thanatos::shared::hex::hex16(op) +
        " len=" + std::to_string(bytes.size() - 2));

    // Hand off payload (without opcode) to the flow state machine.
    // オペコードを除いたペイロードをフローステートマシンへ引き渡す。
    flow_->handle(op, bytes.data() + 2, bytes.size() - 2);
}

}  // namespace ro
}  // namespace interface
}  // namespace thanatos
}  // namespace arkan
