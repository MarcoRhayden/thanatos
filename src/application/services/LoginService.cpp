#include <memory>
#include <string>
#include <vector>

#include "ILoginService.hpp"
#include "domain/protocol/Packet.hpp"
#include "infrastructure/config/Config.hpp"

namespace arkan::poseidon::application::services
{

namespace
{
constexpr std::uint16_t OPC_LOGIN_REQ = 0x0100;    // dev placeholder
constexpr std::uint16_t OPC_LOGIN_OK = 0x0101;     // dev
constexpr std::uint16_t OPC_SERVER_LIST = 0x0102;  // dev
}  // namespace

class LoginService final : public ILoginService
{
   public:
    explicit LoginService(const infrastructure::config::Config& cfg) : cfg_(cfg) {}

    std::vector<domain::protocol::Packet> handle(const domain::protocol::Packet& in) override
    {
        if (in.opcode != OPC_LOGIN_REQ) return {};  // ignora outros por enquanto

        std::vector<domain::protocol::Packet> out;

        // Login OK (qualquer credencial)
        domain::protocol::Packet ok;
        ok.opcode = OPC_LOGIN_OK;
        ok.payload = {0xAA, 0xAA, 0xAA, 0xAA};  // session/account fake, só placeholder
        out.push_back(std::move(ok));

        // Server list com 1 entrada (nosso Char local)
        domain::protocol::Packet sl;
        sl.opcode = OPC_SERVER_LIST;
        // payload dev: ip (127.0.0.1), port (LE16), name("Poseidon-Char")
        sl.payload = {127,
                      0,
                      0,
                      1,
                      static_cast<std::uint8_t>(cfg_.char_port & 0xFF),
                      static_cast<std::uint8_t>((cfg_.char_port >> 8) & 0xFF)};
        static constexpr char name[] = "Poseidon-Char";
        sl.payload.insert(sl.payload.end(), name, name + sizeof(name) - 1);
        out.push_back(std::move(sl));

        return out;
    }

   private:
    infrastructure::config::Config cfg_;
};

std::unique_ptr<ILoginService> MakeLoginService(const infrastructure::config::Config& cfg)
{
    return std::unique_ptr<ILoginService>(new LoginService(cfg));
}

}  // namespace arkan::poseidon::application::services
