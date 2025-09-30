#pragma once

#include <atomic>

namespace arkan
{
namespace poseidon
{
namespace interface
{
namespace ro
{

// Signal: after 0069/0AC4, the next connect is from CHAR (map login)
inline std::atomic<bool> g_expect_char_on_next_connect{false};

}  // namespace ro
}  // namespace interface
}  // namespace poseidon
}  // namespace arkan
