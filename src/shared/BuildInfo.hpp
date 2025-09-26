#pragma once

#include <string_view>

namespace arkan::poseidon::shared
{

inline constexpr std::string_view kProjectName = "Arkan-Poseidon";
inline constexpr std::string_view kVersion = "0.1.0";
inline constexpr std::string_view kBuildProfile =
#if defined(NDEBUG)
    "Release";
#else
    "Debug";
#endif

}  // namespace arkan::poseidon::shared
