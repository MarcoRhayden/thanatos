#include <boost/asio.hpp>

#include "application/services/ICharService.hpp"
#include "application/services/ILoginService.hpp"
#include "application/state/SessionRegistry.hpp"
#include "infrastructure/config/Config.hpp"
#include "infrastructure/log/Logger.hpp"
#include "infrastructure/net/asio/AsioTcpServer.hpp"
#include "interface/dev/CharHandlerDev.hpp"
#include "interface/dev/LoginHandlerDev.hpp"
#include "interface/dev/RoBridgeHandler.hpp"
#include "interface/query/QueryHandler.hpp"
#include "shared/BuildInfo.hpp"

namespace cfg = arkan::poseidon::infrastructure::config;
namespace asio_impl = arkan::poseidon::infrastructure::net::asio_impl;
using arkan::poseidon::infrastructure::log::Logger;

int main()
{
    auto config = cfg::LoadConfig("config/poseidon.toml");

    Logger::init(config.service_name, config.log_level, config.log_to_file, config.log_file,
                 config.log_max_size_bytes, config.log_max_files);

    Logger::info("===========================================");
    Logger::info(std::string("Service: ") + config.service_name);
    Logger::info(std::string("Version: ") + config.version);
    Logger::info(std::string("Profile: ") + std::string(arkan::poseidon::shared::kBuildProfile));
    Logger::info(std::string("Config:  ") + config.loaded_from);
    Logger::info("===========================================");

    boost::asio::io_context io;

    // Services
    auto login_svc_up = arkan::poseidon::application::services::MakeLoginService(config);
    std::shared_ptr<arkan::poseidon::application::services::ILoginService> login_svc =
        std::move(login_svc_up);

    auto char_svc_up = arkan::poseidon::application::services::MakeCharService(config);
    std::shared_ptr<arkan::poseidon::application::services::ICharService> char_svc =
        std::move(char_svc_up);

    // Shared Registry
    auto registry = std::make_shared<arkan::poseidon::application::state::SessionRegistry>();

    // Handlers
    auto login_handler = std::make_shared<arkan::poseidon::interface::dev::LoginHandlerDev>(
        login_svc, config.proto_max_packet);
    auto char_handler =
        std::make_shared<arkan::poseidon::interface::dev::CharHandlerDev>(char_svc, registry);
    auto ro_bridge = std::make_shared<arkan::poseidon::interface::dev::RoBridgeHandler>(
        io, config.ro_host, config.ro_port, registry);
    auto query_handler = std::make_shared<arkan::poseidon::interface::query::QueryHandler>(
        registry, config.query_max_buf);

    // Servers
    auto login_srv = asio_impl::MakeTcpServer(io, config.login_port, login_handler);
    auto char_srv = asio_impl::MakeTcpServer(io, config.char_port, char_handler);
    auto ro_srv = asio_impl::MakeTcpServer(io, config.ro_port, ro_bridge);
    auto query_srv = asio_impl::MakeTcpServer(io, config.openkore_port, query_handler);

    login_srv->start();
    char_srv->start();
    ro_srv->start();
    query_srv->start();

    Logger::info("Login on " + std::to_string(config.login_port) + ", Char on " +
                 std::to_string(config.char_port) + ", RO on " + std::to_string(config.ro_port) +
                 ", Query on " + std::to_string(config.openkore_port));

    io.run();
}
