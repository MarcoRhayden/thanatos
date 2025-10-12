#pragma once

#include <cstdint>
#include <span>

namespace arkan::thanatos::application::ports::crypto
{
struct IRng
{
    virtual ~IRng() = default;
    virtual bool random(std::span<uint8_t> out) = 0;
};
}  // namespace arkan::thanatos::application::ports::crypto
