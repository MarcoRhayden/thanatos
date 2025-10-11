#include "interface/ragnarok/char/CharHandler.hpp"

#include <cstring>

#include "interface/ragnarok/char/CharFlow.hpp"
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
   CharHandler
   - Binds a network session to the Ragnarok "Char Server" flow (CharFlow).
   - Receives raw client packets and forwards them to the flow state machine.
   - ネットワークセッションをラグナロクの「キャラ鯖」フロー(CharFlow)へ接続する。
   - クライアントからの生パケットを受け取り、フローのステートマシンへ渡す。
----------------------------------------------------------------------------- */

CharHandler::~CharHandler() = default;

/* -----------------------------------------------------------------------------
   ctor
   - Extracts IP/port from app config and seeds initial protocol state.
   - アプリ設定からIP/ポートを取り出し、初期プロトコル状態を用意する。
----------------------------------------------------------------------------- */
CharHandler::CharHandler(std::shared_ptr<SessionRegistry> /*registry*/, const apc::Config& cfg,
                         OnLogFn logger)
    : log_(std::move(logger))
{
    // Resolve char/map server IP (IPv4) from textual config
    // 文字列の設定値からIPv4アドレスを解決
    boost::system::error_code ec;
    auto addr = boost::asio::ip::make_address_v4(cfg.ro_host, ec);
    if (!ec)
    {
        auto bytes = addr.to_bytes();
        cfg_.mapIp = {bytes[0], bytes[1], bytes[2], bytes[3]};
    }

    // Store protocol-facing settings (little-endian port, fixed char list block size)
    // プロトコル側設定の格納（LEポート、キャラ一覧ブロックサイズ）
    cfg_.mapPortLE = static_cast<uint16_t>(cfg.char_port);
    cfg_.charBlockSize = 155;

    // Seed some deterministic IDs for local testing (will be overwritten during flow)
    // ローカルテスト用の初期ID（フロー中に上書きされる想定）
    st_.accountID = {0x81, 0x84, 0x1E, 0x00};
    st_.selectedCharID = {0xE9, 0x03, 0x00, 0x00};

    // Create the CharFlow orchestrator that builds and sends server packets
    // サーバーパケットを生成・送信するCharFlowを生成
    flow_ = std::make_unique<charflow::CharFlow>(
        cfg_, st_,
        /* send lambda: writes a finished Packet to the active socket */
        /* 送信ラムダ: 生成済みパケットをアクティブソケットへ書き出す */
        [this](const std::vector<uint8_t>& p) { sendBytes(p); },
        /* log lambda: forwards text logs to provided sink */
        /* ログラムダ: テキストログを指定の出力先へ転送 */
        [this](const std::string& m) { log(m); });
}

/* -----------------------------------------------------------------------------
   log
   - Thin wrapper to call the injected logger if present.
   - 依存注入されたロガーがあれば呼び出すラッパ。
----------------------------------------------------------------------------- */
void CharHandler::log(const std::string& m)
{
    if (log_) log_(m);
}

/* -----------------------------------------------------------------------------
   sendBytes
   - Sends raw bytes to the current session (if any).
   - 現在のセッションがあれば、生バイト列を送信する。
----------------------------------------------------------------------------- */
void CharHandler::sendBytes(const std::vector<uint8_t>& p)
{
    if (auto s = cur_.lock()) s->send(std::span<const uint8_t>(p.data(), p.size()));
}

/* -----------------------------------------------------------------------------
   on_connect
   - Called when a client socket connects.
   - Optionally binds the “wire bridge” (GG/guard) to the new session.
   - クライアントが接続したときに呼ばれる。
   - 必要に応じて“ワイヤブリッジ”(GG/ガード)をこのセッションへ接続する。
----------------------------------------------------------------------------- */
void CharHandler::on_connect(std::shared_ptr<ports_net::ISession> s)
{
    cur_ = s;
    log(std::string("connected: ") + s->remote_endpoint());

    // Bind the session to guard/bridge if present
    // ガード／ブリッジが有ればバインド
    if (gg_)
    {
        if (!wire_)
            wire_ = std::make_unique<wire::SessionWire>(cur_);
        else
            wire_->reset(cur_);

        gg_->bindClientWire(wire_.get());
    }

    // Informational flag set by LoginFlow to indicate handoff
    // LoginFlowからの引き継ぎを示すフラグ（情報ログのみ）
    if (g_expect_char_on_next_connect.exchange(false))
    {
        log("came from LoginFlow (ok)");
    }
}

/* -----------------------------------------------------------------------------
   on_disconnect
   - Called when the client socket closes or errors out.
   - Unbinds guard/bridge and clears the current session.
   - クライアントが切断・エラー時に呼ばれる。
   - ガード／ブリッジを解除し、現在のセッションをクリア。
----------------------------------------------------------------------------- */
void CharHandler::on_disconnect(std::shared_ptr<ports_net::ISession> s, const std::error_code&)
{
    (void)s;
    cur_.reset();

    // Unbind bridge if it exists
    // ブリッジがあれば解除
    if (gg_) gg_->bindClientWire(nullptr);

    log("disconnected");
}

/* -----------------------------------------------------------------------------
   on_data
   - Entry point for every inbound client packet.
   - First gives GG/guard a chance to consume it; if not consumed, decode opcode
     and forward the payload to CharFlow::handle.
   - すべての受信パケットの入口。
   - まずGG/ガードに処理機会を与え、未消費ならオペコードを読み出して
     CharFlow::handle へ渡す。
----------------------------------------------------------------------------- */
void CharHandler::on_data(std::shared_ptr<ports_net::ISession> s,
                          std::span<const std::uint8_t> bytes)
{
    cur_ = s;

    // Give guard/bridge a chance to fully consume the packet (anti-cheat/keepalive etc.)
    // ガード／ブリッジ側でパケットを消費するか確認（アンチチート／キープアライブ等）
    if (gg_ && gg_->maybe_consume_c2s(bytes.data(), bytes.size()))
    {
        return;
    }

    // All valid packets must have at least an opcode (2 bytes)
    // 正常なパケットは最低2バイトのオペコードを持つ
    if (bytes.size() < 2) return;

    const uint16_t op = rd16le(bytes.data());
    const uint8_t* pl = bytes.data() + 2;  // payload start / ペイロード開始
    const size_t ln = bytes.size() - 2;    // payload length / ペイロード長

    /* ---- Minimal GG/keepalive echo stubs -----------------------------------
       Some clients expect these immediate echoes; keep them lightweight and
       non-blocking. If pattern matches, echo and short-circuit.
       一部クライアントは即時エコーを期待する。マッチしたらエコーして終了。
    ------------------------------------------------------------------------- */
    auto echo = [this](uint16_t opcode, const uint8_t* p, size_t n)
    {
        std::vector<uint8_t> out;
        wr16le(out, opcode);
        out.insert(out.end(), p, p + n);
        sendBytes(out);
    };

    switch (op)
    {
        case 0x007D:  // single-byte keepalive
            // 1バイトのキープアライブ
            if (ln == 1)
            {
                echo(0x007D, pl, ln);
                log(std::string("[char] gg-stub: echo 007D (len=1) v=") + std::to_string(pl[0]));
                return;
            }
            break;

        case 0x0360:  // 5-byte guard ping
            // 5バイトのガード用PING
            if (ln == 5)
            {
                echo(0x0360, pl, ln);
                log(std::string("[char] gg-stub: echo 0360 (len=5) payload=") + hex::hex(pl, ln));
                return;
            }
            break;

        case 0x08C9:  // single-byte ping
            // 1バイトのPING
            if (ln == 1)
            {
                echo(0x08C9, pl, ln);
                log(std::string("[char] gg-stub: echo 08C9 (len=1) v=") + std::to_string(pl[0]));
                return;
            }
            break;

        default:
            break;
    }
    /* ---- /GG/keepalive stubs --------------------------------------------- */

    // Forward the remaining game packet to the flow state machine
    // 残りのゲームパケットをフローステートマシンへ渡す
    flow_->handle(op, bytes.data() + 2, bytes.size() - 2);
}

}  // namespace ro
}  // namespace interface
}  // namespace thanatos
}  // namespace arkan
