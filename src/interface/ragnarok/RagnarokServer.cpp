#include "interface/ragnarok/RagnarokServer.hpp"

namespace arkan::poseidon::interface::ro
{

RagnarokServer::RagnarokServer(boost::asio::io_context& io,
                               std::shared_ptr<SessionRegistry> registry, const apc::Config& cfg)
    : registry_(std::move(registry)),
      cfg_(cfg),
      login_handler_(std::make_shared<LoginHandler>(
          registry_, cfg_, [](const std::string& msg) { Logger::debug("[login] " + msg); })),
      char_handler_(std::make_shared<CharHandler>(
          registry_, cfg_, [](const std::string& msg) { Logger::debug("[char] " + msg); })),
      login_srv_(asio_impl::MakeTcpServer(io, cfg_.login_port, login_handler_, cfg_.fakeIP)),
      char_srv_(asio_impl::MakeTcpServer(io, cfg_.char_port, char_handler_, cfg_.fakeIP))
{
    Logger::debug("[ragnarok] RagnarokServer constructed: login=" +
                  std::to_string(cfg_.login_port) + " char=" + std::to_string(cfg_.char_port));
}

void RagnarokServer::start()
{
    if (login_srv_) login_srv_->start();
    if (char_srv_) char_srv_->start();
    Logger::debug("[ragnarok] RagnarokServer start(): listeners active");
}

void RagnarokServer::iterate()
{
    // no-op for now; hook point for timers/housekeeping
}

void RagnarokServer::stop()
{
    if (login_srv_) login_srv_->stop();
    if (char_srv_) char_srv_->stop();
    Logger::debug("[ragnarok] RagnarokServer stop(): listeners closed");
}

}  // namespace arkan::poseidon::interface::ro
