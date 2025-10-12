#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>

#include "application/ports/crypto/IRng.hpp"
#include "application/ports/crypto/ITokenGenerator.hpp"
#include "infrastructure/crypto/Base64Url.hpp"

namespace arkan::thanatos::infrastructure::crypto
{

class Random16TokenGenerator final
    : public arkan::thanatos::application::ports::crypto::ITokenGenerator
{
    arkan::thanatos::application::ports::crypto::IRng& rng_;

   public:
    explicit Random16TokenGenerator(arkan::thanatos::application::ports::crypto::IRng& rng)
        : rng_(rng)
    {
    }

    std::string makeLoginToken() override
    {
        std::array<std::uint8_t, 12> buf{};
        if (!rng_.random(std::span<std::uint8_t>(buf)))
        {
            throw std::runtime_error("RNG failure");
        }
        return b64url_12(buf.data());
    }
};

}  // namespace arkan::thanatos::infrastructure::crypto
