#pragma once

// clang-format off
#include <windows.h>
#include <bcrypt.h>
// clang-format on

#include <cstdint>
#include <span>

#include "application/ports/crypto/IRng.hpp"

namespace arkan::thanatos::infrastructure::crypto
{
class WinCngRng final : public arkan::thanatos::application::ports::crypto::IRng
{
   public:
    bool random(std::span<std::uint8_t> out) override
    {
        const NTSTATUS st = ::BCryptGenRandom(
            /*hAlgorithm*/ nullptr,
            /*pbBuffer*/ out.data(),
            /*cbBuffer*/ static_cast<ULONG>(out.size()),
            /*dwFlags*/ BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        return st == 0;  // STATUS_SUCCESS
    }
};
}  // namespace arkan::thanatos::infrastructure::crypto
