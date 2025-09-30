#include "LoginFlow.hpp"

#include <string>

#include "interface/ragnarok/model/PhaseSignal.hpp"
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

using namespace arkan::poseidon::interface::ro::proto;
using namespace arkan::poseidon::interface::ro::proto::login;

LoginFlow::LoginFlow(LoginCfg& cfg, LoginState& st, SendFn send, LogFn log)
    : cfg_(cfg), st_(st), send_(std::move(send)), log_(std::move(log))
{
}

void LoginFlow::handle(uint16_t opcode, const uint8_t* /*data*/, size_t /*len*/)
{
    switch (opcode)
    {
        // ===== secure handshake =====
        case 0x01DB:
        case 0x0204:
            onSecureHandshake();
            return;

        // ===== token =====
        case 0x0ACF:
        case 0x0C26:
            onTokenRequest();
            return;

        // ===== master login (several variants) =====
        case 0x0064:
        case 0x01DD:
        case 0x01FA:
        case 0x0AAC:
        case 0x0B04:
        case 0x0987:
        case 0x0A76:
        case 0x2085:
        case 0x2B0D:
        case 0x1DD5:
        case 0x0825:
            onMasterLogin(opcode);
            return;

        default:
            log_(std::string("unhandled opcode=0x") + std::to_string(opcode));
    }
}

void LoginFlow::onSecureHandshake()
{
    log_("secure login handshake request");
    send_(SecureLoginKey01DC());
}

void LoginFlow::onTokenRequest()
{
    log_("token request");
    send_(LoginToken0AE3());
    awaiting_master_ = true;
}

void LoginFlow::onMasterLogin(uint16_t opcode)
{
    if (!awaiting_master_)
    {
        log_("master login received but not awaiting; ignoring");
        return;
    }
    awaiting_master_ = false;
    st_.lastMasterOpcode = opcode;

    // fixed IDs
    st_.accountID = {0x81, 0x84, 0x1E, 0x00};
    st_.sessionID = {0x00, 0x5E, 0xD0, 0xB2};
    st_.sessionID2 = {0xFF, 0x00, 0x00, 0x00};

    log_("master login (auto) opcode=0x" + std::to_string(opcode) + " -> account_server_info");

    const bool needs_0AC4 =
        (opcode == 0x0825 || opcode == 0x2085 || opcode == 0x2B0D || opcode == 0x1DD5);

    if (needs_0AC4 || !cfg_.prefer0069)
    {
        send_(AccountServer0AC4(st_.sessionID, st_.accountID, st_.sessionID2, cfg_.hostIp,
                                cfg_.hostPortLE, cfg_.serverName, cfg_.usersOnline, cfg_.male));
        log_("-> 0x0AC4 (new format) sent");
    }
    else
    {
        // classic format (0x0069)
        send_(AccountServer0069(st_.sessionID, st_.accountID, st_.sessionID2, cfg_.hostIp,
                                cfg_.hostPortLE, cfg_.serverName, cfg_.usersOnline, cfg_.male));
        log_("-> 0x0069 (classic) sent");
    }

    // Signals that the next connect will be CHAR
    arkan::poseidon::interface::ro::g_expect_char_on_next_connect.store(true);
    log_("signaled g_expect_char_on_next_connect = true");
}

}  // namespace loginflow
}  // namespace ro
}  // namespace interface
}  // namespace poseidon
}  // namespace arkan
