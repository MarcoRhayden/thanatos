#include "Codec.hpp"

#include <chrono>

namespace arkan::poseidon::interface::ro::proto
{

uint32_t tick_ms()
{
    using namespace std::chrono;
    static const auto t0 = steady_clock::now();
    return (uint32_t)duration_cast<milliseconds>(steady_clock::now() - t0).count();
}

}  // namespace arkan::poseidon::interface::ro::proto
