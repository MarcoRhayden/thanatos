#pragma once
#include <cstdint>
#include <span>
#include <vector>

#include "Packet.hpp"

namespace arkan::poseidon::domain::protocol
{

class Parser
{
   public:
    explicit Parser(std::size_t max_packet = 4 * 1024 * 1024) : max_(max_packet) {}

    // Alimenta bytes brutos (pode vir fragmentado ou coalescido)
    inline void feed(std::span<const std::uint8_t> bytes)
    {
        buf_.insert(buf_.end(), bytes.begin(), bytes.end());

        for (;;)
        {
            if (buf_.size() < 4) return;
            const auto size = ReadLE16(buf_.data() + 2);
            if (size < 4 || size > max_)
            {
                // pacote inválido -> descarta tudo (defensivo)
                buf_.clear();
                ready_.clear();
                return;
            }
            if (buf_.size() < size) return;  // espera completar

            Packet p;
            p.opcode = ReadLE16(buf_.data());
            p.payload.assign(buf_.begin() + 4, buf_.begin() + size);
            ready_.push_back(std::move(p));
            buf_.erase(buf_.begin(), buf_.begin() + size);
        }
    }

    // Retorna todos os pacotes completos acumulados e limpa a fila
    inline std::vector<Packet> drain()
    {
        auto out = std::move(ready_);
        ready_.clear();
        return out;
    }

    inline void set_max(std::size_t m)
    {
        max_ = m;
    }

   private:
    static inline std::uint16_t ReadLE16(const std::uint8_t* p)
    {
        return static_cast<std::uint16_t>(p[0] | (static_cast<std::uint16_t>(p[1]) << 8));
    }

    std::vector<std::uint8_t> buf_;
    std::vector<Packet> ready_;
    std::size_t max_;
};

}  // namespace arkan::poseidon::domain::protocol
