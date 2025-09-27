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

struct ICharService
{
    virtual ~ICharService() = default;
    virtual std::vector<domain::protocol::Packet> handle(const domain::protocol::Packet& in) = 0;
};

std::unique_ptr<ICharService> MakeCharService(
    const arkan::poseidon::infrastructure::config::Config& cfg);

}  // namespace arkan::poseidon::application::services
