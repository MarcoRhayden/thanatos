#include "interface/ragnarok/RagnarokServer.hpp"

namespace arkan::poseidon::interface::ro
{

RagnarokServer::RagnarokServer(boost::asio::io_context& io,
                               std::shared_ptr<SessionRegistry> registry, const apc::Config& cfg)
    : io_(io),
      registry_(std::move(registry)),
      cfg_(cfg),
      qhost_(cfg_.fakeIP),
      qport_(cfg_.query_port),
      login_handler_(std::make_shared<LoginHandler>(
          registry_, cfg_, [](const std::string& m) { Logger::debug("[login] " + m); })),
      char_handler_(std::make_shared<CharHandler>(
          registry_, cfg_, [](const std::string& m) { Logger::debug("[char] " + m); })),
      login_srv_(asio_impl::MakeTcpServer(io_, cfg_.login_port, login_handler_, cfg_.fakeIP)),
      char_srv_(asio_impl::MakeTcpServer(io_, cfg_.char_port, char_handler_, cfg_.fakeIP))
{
    query_server_ = std::make_unique<interface::query::QueryServer>(io_, qhost_, qport_);

    // Bridge GameGuard (injects into handlers)
    gg_bridge_ = std::make_unique<application::services::GameGuardBridge>(*query_server_);
    login_handler_->set_gg_bridge(gg_bridge_.get());
    char_handler_->set_gg_bridge(gg_bridge_.get());

    Logger::debug("[ragnarok] constructed: login=" + std::to_string(cfg_.login_port) +
                  " char=" + std::to_string(cfg_.char_port) + " query=" + qhost_ + ":" +
                  std::to_string(qport_));
}

void RagnarokServer::start()
{
    // Start the Poseidon channel
    if (query_server_) query_server_->start();
    if (login_srv_) login_srv_->start();
    if (char_srv_) char_srv_->start();
    Logger::debug("[ragnarok] start(): listeners active");
}

void RagnarokServer::stop()
{
    if (login_srv_) login_srv_->stop();
    if (char_srv_) char_srv_->stop();
    if (query_server_) query_server_->stop();
    Logger::debug("[ragnarok] stop(): listeners closed");
}

}  // namespace arkan::poseidon::interface::ro
