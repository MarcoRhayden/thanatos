#pragma once

#include <memory>
#include <vector>

#include "domain/protocol/Packet.hpp"

namespace arkan::poseidon::infrastructure::config
{
struct Config;
}

namespace arkan::poseidon::application::services
{
struct ILoginService
{
    virtual ~ILoginService() = default;
    virtual std::vector<arkan::poseidon::domain::protocol::Packet> handle(
        const arkan::poseidon::domain::protocol::Packet& in) = 0;
};

std::unique_ptr<ILoginService> MakeLoginService(
    const arkan::poseidon::infrastructure::config::Config& cfg);
}  // namespace arkan::poseidon::application::services
