#include <boost/asio.hpp>

#include "application/state/SessionRegistry.hpp"
#include "infrastructure/config/Config.hpp"
#include "infrastructure/log/Logger.hpp"
#include "infrastructure/net/asio/AsioTcpServer.hpp"
#include "interface/query/QueryHandler.hpp"
#include "interface/ro/RagnarokServer.hpp"
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
    auto registry = std::make_shared<arkan::poseidon::application::state::SessionRegistry>();

    auto ragnarok_server =
        std::make_shared<arkan::poseidon::interface::ro::RagnarokServer>(io, registry, config);
    ragnarok_server->start();

    auto query_handler = std::make_shared<arkan::poseidon::interface::query::QueryHandler>(
        registry, config.query_max_buf);
    auto query_srv = asio_impl::MakeTcpServer(io, config.openkore_port, query_handler);
    query_srv->start();

    Logger::info("Login on " + std::to_string(config.login_port) + ", Char on " +
                 std::to_string(config.char_port) + ", Query on " +
                 std::to_string(config.openkore_port));

    io.run();
}
