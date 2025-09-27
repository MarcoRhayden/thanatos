#include <memory>
#include <string>
#include <vector>

#include "ICharService.hpp"
#include "domain/protocol/Packet.hpp"
#include "infrastructure/config/Config.hpp"

namespace arkan::poseidon::application::services
{

namespace
{
constexpr std::uint16_t OPC_CHAR_LIST = 0x0200;    // dev
constexpr std::uint16_t OPC_CHAR_SELECT = 0x0201;  // dev
constexpr std::uint16_t OPC_CHAR_OK = 0x0202;      // dev (redirect to RO)
}  // namespace

class CharService final : public ICharService
{
   public:
    explicit CharService(const infrastructure::config::Config& cfg) : cfg_(cfg) {}

    std::vector<domain::protocol::Packet> handle(const domain::protocol::Packet& in) override
    {
        std::vector<domain::protocol::Packet> out;

        if (in.opcode == OPC_CHAR_LIST)
        {
            // Returns 1 synthetic "novice"
            domain::protocol::Packet list;
            list.opcode = OPC_CHAR_LIST;
            const std::string name = cfg_.dummy_char_name.empty() ? "Novice" : cfg_.dummy_char_name;
            list.payload.assign(name.begin(), name.end());
            out.push_back(std::move(list));
        }
        else if (in.opcode == OPC_CHAR_SELECT)
        {
            // Redirect to RO (127.0.0.1:ro_port)
            domain::protocol::Packet ok;
            ok.opcode = OPC_CHAR_OK;
            ok.payload = {127,
                          0,
                          0,
                          1,
                          static_cast<std::uint8_t>(cfg_.ro_port & 0xFF),
                          static_cast<std::uint8_t>((cfg_.ro_port >> 8) & 0xFF)};
            out.push_back(std::move(ok));
        }

        return out;
    }

   private:
    infrastructure::config::Config cfg_;
};

std::unique_ptr<ICharService> MakeCharService(const infrastructure::config::Config& cfg)
{
    return std::unique_ptr<ICharService>(new CharService(cfg));
}

}  // namespace arkan::poseidon::application::services
