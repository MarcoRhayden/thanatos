// Minimal RO server facade that starts login/char listeners and a Poseidon-style query server.
// ログイン/キャラ用のリスナーと Poseidon 互換のクエリサーバを起動する薄いファサード

#include "interface/ragnarok/RagnarokServer.hpp"

#include "application/services/GameGuardBridge.hpp"

// Handy aliases to keep code tidy
// 可読性のための別名
using arkan::thanatos::application::services::GameGuardBridge;
using GGStrategy = GameGuardBridge::GGStrategy;

namespace arkan::thanatos::interface::ro
{

// ctor
RagnarokServer::RagnarokServer(boost::asio::io_context& io,
                               std::shared_ptr<SessionRegistry> registry, const apc::Config& cfg)
    : io_(io),
      registry_(std::move(registry)),
      cfg_(cfg),
      // Bind hosts from config into local members so the ctor body can use stable values.
      // コンフィグからホスト名をローカルメンバへ束縛（以降は安定した値を使用）
      roHost_(cfg_.ro_host),
      qhost_(cfg_.query_host),
      qport_(cfg_.query_port),
      // Handlers (inject logger adapter)
      // ハンドラ（ロガーアダプタを注入）
      login_handler_(std::make_shared<LoginHandler>(
          registry_, cfg_, [](const std::string& m) { Logger::debug("[login] " + m); })),
      char_handler_(std::make_shared<CharHandler>(
          registry_, cfg_, [](const std::string& m) { Logger::debug("[char] " + m); })),
      // TCP servers bind to RO host (roHost_) and respective ports
      // 各TCPサーバは RO ホスト(roHost_) とポートへバインド
      login_srv_(asio_impl::MakeTcpServer(io_, cfg_.login_port, login_handler_, roHost_)),
      char_srv_(asio_impl::MakeTcpServer(io_, cfg_.char_port, char_handler_, roHost_))
{
    // Thanatos/BUS Query server uses dedicated query host/port
    // Thanatos/BUS 用クエリサーバは専用のホスト/ポートを使用
    query_server_ = std::make_unique<interface::query::QueryServer>(io_, qhost_, qport_);

    // GameGuard bridge (shared service) -> inject to handlers
    // GameGuard ブリッジ（共有サービス） -> 各ハンドラへ注入
    gg_bridge_ = std::make_unique<GameGuardBridge>(*query_server_);

    // Tweakables
    // 調整パラメータ
    gg_bridge_->set_size_bounds(2, 4096);
    gg_bridge_->set_timeout(std::chrono::milliseconds(3000));
    gg_bridge_->set_greedy_window(std::chrono::milliseconds(150));

    // Strategy:
    // - decide by last 09CF length (72 -> BODY_TRUNC_18, 80 -> FULL_FRAME by default)
    // - 直前の09CF長で自動選択（既定は 72 -> BODY_TRUNC_18, 80 -> FULL_FRAME）
    gg_bridge_->set_strategy(GGStrategy::AUTO);
    // gg_bridge_->set_strategy(GGStrategy::BODY_TRUNC_18);
    // gg_bridge_->set_strategy(GGStrategy::FULL_FRAME);
    // gg_bridge_->set_strategy(GGStrategy::BODY_LEN);

    // Wire bridge into handlers
    // ハンドラへブリッジを配線
    login_handler_->set_gg_bridge(gg_bridge_.get());
    char_handler_->set_gg_bridge(gg_bridge_.get());

    Logger::debug("[ragnarok] constructed: login=" + std::to_string(cfg_.login_port) +
                  " char=" + std::to_string(cfg_.char_port) + " query=" + qhost_ + ":" +
                  std::to_string(qport_));
}

// Start all listeners
// 全リスナー起動
void RagnarokServer::start()
{
    if (query_server_) query_server_->start();  // start query server first / 先にクエリを起動
    if (login_srv_) login_srv_->start();
    if (char_srv_) char_srv_->start();
    Logger::debug("[ragnarok] start(): listeners active");
}

// Stop all listeners
// 全リスナー停止
void RagnarokServer::stop()
{
    if (login_srv_) login_srv_->stop();
    if (char_srv_) char_srv_->stop();
    if (query_server_) query_server_->stop();
    Logger::debug("[ragnarok] stop(): listeners closed");
}

}  // namespace arkan::thanatos::interface::ro
