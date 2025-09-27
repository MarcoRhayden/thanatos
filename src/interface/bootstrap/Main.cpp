#include <boost/asio.hpp>
#include <boost/asio/executor_work_guard.hpp>

#include "application/services/ICharService.hpp"
#include "application/services/ILoginService.hpp"
#include "infrastructure/config/Config.hpp"
#include "infrastructure/log/Logger.hpp"
#include "infrastructure/net/asio/AsioTcpServer.hpp"
#include "interface/dev/CharHandlerDev.hpp"
#include "interface/dev/LoginHandlerDev.hpp"
#include "interface/dev/RoBridgeHandler.hpp"
#include "shared/BuildInfo.hpp"

namespace cfg = arkan::poseidon::infrastructure::config;
namespace asio_impl = arkan::poseidon::infrastructure::net::asio_impl;
using arkan::poseidon::infrastructure::log::Logger;

int main()
{
    try
    {
        auto config = cfg::LoadConfig("config/poseidon.toml");

        Logger::init(config.service_name, config.log_level, config.log_to_file, config.log_file,
                     config.log_max_size_bytes, config.log_max_files);

        Logger::info("===========================================");
        Logger::info(std::string("Service: ") + config.service_name);
        Logger::info(std::string("Version: ") + config.version);
        Logger::info(std::string("Profile: ") +
                     std::string(arkan::poseidon::shared::kBuildProfile));
        Logger::info(std::string("Config:  ") + config.loaded_from);
        Logger::info("===========================================");

        boost::asio::io_context io;
        auto guard = boost::asio::make_work_guard(io.get_executor());

        // Services
        auto loginSvc = arkan::poseidon::application::services::MakeLoginService(config);
        auto charSvc = arkan::poseidon::application::services::MakeCharService(config);

        // Handlers
        auto loginH = std::make_shared<arkan::poseidon::interface::dev::LoginHandlerDev>(
            std::move(loginSvc), config.proto_max_packet);
        auto charH = std::make_shared<arkan::poseidon::interface::dev::CharHandlerDev>(
            std::move(charSvc), config.proto_max_packet);
        auto roH = std::make_shared<arkan::poseidon::interface::dev::RoBridgeHandler>(
            io, config.openkore_host, config.openkore_port);

        // Servers (login/char/ro)
        auto loginSrv = asio_impl::MakeTcpServer(io, config.login_port, loginH);
        auto charSrv = asio_impl::MakeTcpServer(io, config.char_port, charH);
        auto roSrv = asio_impl::MakeTcpServer(io, config.ro_port, roH);

        loginSrv->start();
        charSrv->start();
        roSrv->start();

        Logger::info("Poseidon classic mode UP:");
        Logger::info("  Login  : 127.0.0.1:" + std::to_string(config.login_port));
        Logger::info("  Char   : 127.0.0.1:" + std::to_string(config.char_port));
        Logger::info("  RO     : 127.0.0.1:" + std::to_string(config.ro_port));
        Logger::info("  OK bridge -> " + config.openkore_host + ":" +
                     std::to_string(config.openkore_port));

        io.run();
        return 0;
    }
    catch (const std::exception& e)
    {
        Logger::error(std::string("Fatal error: ") + e.what());
        return 1;
    }
}
