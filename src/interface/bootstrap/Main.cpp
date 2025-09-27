#include <boost/asio.hpp>

#include "application/services/ICharService.hpp"
#include "application/services/ILoginService.hpp"
#include "infrastructure/config/Config.hpp"
#include "infrastructure/log/Logger.hpp"
#include "infrastructure/net/asio/AsioTcpServer.hpp"
#include "interface/dev/CharHandlerDev.hpp"
#include "interface/dev/LoginHandlerDev.hpp"
#include "shared/BuildInfo.hpp"

namespace cfg = arkan::poseidon::infrastructure::config;
namespace asio_impl = arkan::poseidon::infrastructure::net::asio_impl;
namespace dev = arkan::poseidon::interface::dev;
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

    // services
    auto login_svc = arkan::poseidon::application::services::MakeLoginService(config);
    auto char_svc = arkan::poseidon::application::services::MakeCharService(config);

    // handlers
    auto login_handler = std::make_shared<dev::LoginHandlerDev>(std::move(login_svc));
    auto char_handler = std::make_shared<dev::CharHandlerDev>(std::move(char_svc));

    // servers
    auto login_srv = asio_impl::MakeTcpServer(io, config.login_port, login_handler);
    auto char_srv = asio_impl::MakeTcpServer(io, config.char_port, char_handler);

    login_srv->start();
    char_srv->start();

    Logger::info("Login listening on " + std::to_string(config.login_port) + ", Char on " +
                 std::to_string(config.char_port));

    io.run();
}
