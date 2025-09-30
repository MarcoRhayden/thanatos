#pragma once
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace arkan::poseidon::application::ports::query
{

// Tipos de mensagem no canal Poseidon (compatível com o bridge PSDN).
enum class MsgType : std::uint16_t
{
    PoseidonQuery = 0x0051,  // OpenKore -> Poseidon (payload = GG challenge)
    PoseidonReply = 0x0052   // Poseidon -> OpenKore (payload = GG reply do cliente RO)
};

// Porta de alto nível para o servidor de Query (sem dependência de rede/Asio).
struct IQueryServer
{
    virtual ~IQueryServer() = default;

    // Inicia/para a escuta no canal de query.
    virtual void start() = 0;
    virtual void stop() = 0;

    // Registra callback para quando chegar PoseidonQuery (GG challenge).
    virtual void onQuery(std::function<void(std::vector<std::uint8_t>)> cb) = 0;

    // Envia PoseidonReply de volta ao produtor do query (ex.: OpenKore).
    virtual void sendReply(const std::vector<std::uint8_t>& gg_reply) = 0;

    // (Opcional) Identificação útil para logs.
    virtual std::string endpoint_description() const = 0;
};

}  // namespace arkan::poseidon::application::ports::query
