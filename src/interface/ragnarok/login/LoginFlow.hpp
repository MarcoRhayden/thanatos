#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include "interface/ragnarok/proto/Codec.hpp"
#include "interface/ragnarok/proto/LoginMessages.hpp"

namespace arkan
{
namespace poseidon
{
namespace interface
{
namespace ro
{
namespace loginflow
{

using arkan::poseidon::interface::ro::proto::Packet;

struct LoginCfg
{
    std::array<uint8_t, 4> hostIp{127, 0, 0, 1};
    uint16_t hostPortLE{6900};
    std::string serverName{"Arkan Software"};
    uint32_t usersOnline{77};
    bool male{true};
    bool prefer0069{true};  // if false and master==0x0825, send 0x0AC4
};

struct LoginState
{
    std::array<uint8_t, 4> accountID{0x81, 0x84, 0x1E, 0x00};   // 2000001
    std::array<uint8_t, 4> sessionID{0x00, 0x5E, 0xD0, 0xB2};   // 3000000000
    std::array<uint8_t, 4> sessionID2{0xFF, 0x00, 0x00, 0x00};  // 255
    uint16_t lastMasterOpcode{0};
};

class LoginFlow
{
   public:
    using SendFn = std::function<void(const Packet&)>;
    using LogFn = std::function<void(const std::string&)>;

    LoginFlow(LoginCfg& cfg, LoginState& st, SendFn send, LogFn log);

    // Single event input: opcode + payload
    void handle(uint16_t opcode, const uint8_t* data, size_t len);

   private:
    void onSecureHandshake();
    void onTokenRequest();
    void onMasterLogin(uint16_t opcode);

   private:
    LoginCfg& cfg_;
    LoginState& st_;
    SendFn send_;
    LogFn log_;
    bool awaiting_master_ = false;
};

}  // namespace loginflow
}  // namespace ro
}  // namespace interface
}  // namespace poseidon
}  // namespace arkan
