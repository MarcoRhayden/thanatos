#include "interface/ragnarok/RagnarokServer.hpp"

#include <stdexcept>

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
      // Handlers (inject logger adapter)
      // ハンドラ（ロガーアダプタを注入）
      login_handler_(std::make_shared<LoginHandler>(
          registry_, cfg_, [](const std::string& m) { Logger::debug("[login] " + m); })),
      char_handler_(std::make_shared<CharHandler>(
          registry_, cfg_, [](const std::string& m) { Logger::debug("[char] " + m); })),
      login_srv_(nullptr),
      char_srv_(nullptr),
      query_server_(nullptr),
      gg_bridge_(nullptr)
{
}

// Start all listeners
// 全リスナー起動
void RagnarokServer::start()
{
    const std::size_t n =
        std::min({cfg_.login_ports.size(), cfg_.char_ports.size(), cfg_.query_ports.size()});

    if (n == 0)
        throw std::runtime_error(
            "No port sets configured: login_ports/char_ports/query_ports must be non-empty and "
            "aligned");

    std::exception_ptr last_ex;

    for (std::size_t i = 0; i < n; ++i)
    {
        const uint16_t qport = cfg_.query_ports[i];
        const uint16_t lport = cfg_.login_ports[i];
        const uint16_t cport = cfg_.char_ports[i];

        std::unique_ptr<interface::query::QueryServer> qtmp;
        std::unique_ptr<ports_net::ITcpServer> ltmp;
        std::unique_ptr<ports_net::ITcpServer> ctmp;

        try
        {
            // Query
            qtmp = std::make_unique<interface::query::QueryServer>(io_, qhost_, qport);
            qtmp->start();

            // Login
            ltmp = asio_impl::MakeTcpServer(io_, lport, login_handler_, roHost_);
            ltmp->start();

            // Char
            ctmp = asio_impl::MakeTcpServer(io_, cport, char_handler_, roHost_);
            ctmp->start();

            // success: promote to members
            query_server_ = std::move(qtmp);
            login_srv_ = std::move(ltmp);
            char_srv_ = std::move(ctmp);

            // synchronizes ports/state in handlers
            char_handler_->set_listen_port(cport);
            login_handler_->set_char_port_le(cport);

            // bridge and handlers
            gg_bridge_ = std::make_unique<GameGuardBridge>(*query_server_);
            gg_bridge_->set_size_bounds(2, 4096);
            gg_bridge_->set_timeout(std::chrono::milliseconds(3000));
            gg_bridge_->set_greedy_window(std::chrono::milliseconds(150));
            gg_bridge_->set_strategy(GGStrategy::AUTO);

            login_handler_->set_gg_bridge(gg_bridge_.get());
            char_handler_->set_gg_bridge(gg_bridge_.get());

            active_binding_ = BindingInfo{roHost_, qhost_, lport, cport, qport, i};

            return;
        }
        catch (...)
        {
            last_ex = std::current_exception();

            // rollback
            if (ctmp) ctmp->stop();
            if (ltmp) ltmp->stop();
            if (qtmp) qtmp->stop();
        }
    }

    Logger::error("All configured port sets are busy/unavailable");
    if (last_ex) std::rethrow_exception(last_ex);
    throw std::runtime_error("no port set available");
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
