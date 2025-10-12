#pragma once

#include <string>

namespace arkan::thanatos::application::ports::crypto
{
struct ITokenGenerator
{
    virtual ~ITokenGenerator() = default;
    virtual std::string makeLoginToken() = 0;
};
}  // namespace arkan::thanatos::application::ports::crypto
