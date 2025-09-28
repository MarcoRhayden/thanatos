#include "application/services/ILoginService.hpp"
#include "domain/protocol/Codec.hpp"
#include "domain/protocol/PacketIds.hpp"
#include "infrastructure/config/Config.hpp"

namespace proto = arkan::poseidon::domain::protocol;
namespace ids = arkan::poseidon::domain::protocol::ids;
namespace cfgns = arkan::poseidon::infrastructure::config;

namespace arkan::poseidon::application::services
{

class LoginServiceImpl final : public ILoginService
{
   public:
    explicit LoginServiceImpl(const cfgns::Config& c) : cfg_(c) {}

    std::vector<proto::Packet> handle(const proto::Packet& in) override
    {
        std::vector<proto::Packet> out;

        if (in.opcode == ids::C_LOGIN_REQ)
        {
            std::vector<proto::ServerEntry> servers;
            servers.push_back(proto::ServerEntry{
                .name = "Arkan Char", .host = "127.0.0.1", .port = cfg_.char_port});
            out.push_back(proto::Make_LoginOk_ServerList(servers));
        }

        return out;
    }

   private:
    cfgns::Config cfg_;
};

std::unique_ptr<ILoginService> MakeLoginService(const cfgns::Config& cfg)
{
    return std::make_unique<LoginServiceImpl>(cfg);
}

}  // namespace arkan::poseidon::application::services
