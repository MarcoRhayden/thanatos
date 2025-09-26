#include <boost/asio.hpp>
#include <boost/asio/executor_work_guard.hpp>

#include "infrastructure/config/Config.hpp"
#include "infrastructure/log/Logger.hpp"
#include "infrastructure/net/asio/AsioTcpServer.hpp"
#include "interface/dev/EchoHandlers.hpp"
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

        auto query_handler = std::make_shared<arkan::poseidon::interface::dev::EchoHandler>();
        auto ro_handler = std::make_shared<arkan::poseidon::interface::dev::EchoHandlerRo>();

        auto query_server = asio_impl::MakeTcpServer(io, config.query_port, query_handler);
        auto ro_server = asio_impl::MakeTcpServer(io, config.ro_port, ro_handler);

        query_server->start();
        ro_server->start();

        Logger::info("Query on " + std::to_string(config.query_port) + ", RO on " +
                     std::to_string(config.ro_port));

        io.run();
        return 0;
    }
    catch (const std::exception& e)
    {
        Logger::error(std::string("Fatal error: ") + e.what());
        return 1;
    }
}