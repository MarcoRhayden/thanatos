#include "application/services/ICharService.hpp"
#include "domain/protocol/Codec.hpp"
#include "domain/protocol/PacketIds.hpp"
#include "infrastructure/config/Config.hpp"

namespace proto = arkan::poseidon::domain::protocol;
namespace ids = arkan::poseidon::domain::protocol::ids;
namespace cfgns = arkan::poseidon::infrastructure::config;

namespace arkan::poseidon::application::services
{

class CharServiceImpl final : public ICharService
{
   public:
    explicit CharServiceImpl(const cfgns::Config& c) : cfg_(c) {}

    std::vector<proto::Packet> handle(const proto::Packet& in) override
    {
        std::vector<proto::Packet> out;

        switch (in.opcode)
        {
            case ids::C_CHAR_LIST_REQ:
            {
                std::vector<proto::Character> list;
                list.push_back(proto::Character{
                    .name = cfg_.dummy_char_name,
                    .map = cfg_.dummy_char_map,
                    .x = static_cast<std::uint16_t>(cfg_.dummy_char_x),
                    .y = static_cast<std::uint16_t>(cfg_.dummy_char_y),
                });
                out.push_back(proto::Make_CharList(list));
                break;
            }
            case ids::C_CHAR_SELECT:
            {
                out.push_back(
                    proto::Make_ConnectToMap(cfg_.ro_host, cfg_.ro_port, cfg_.dummy_char_map,
                                             static_cast<std::uint16_t>(cfg_.dummy_char_x),
                                             static_cast<std::uint16_t>(cfg_.dummy_char_y)));
                break;
            }
            default:
                break;
        }

        return out;
    }

   private:
    cfgns::Config cfg_;
};

std::unique_ptr<ICharService> MakeCharService(const cfgns::Config& cfg)
{
    return std::make_unique<CharServiceImpl>(cfg);
}

}  // namespace arkan::poseidon::application::services
