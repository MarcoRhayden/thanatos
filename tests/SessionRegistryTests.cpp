#include <gtest/gtest.h>

#include <atomic>
#include <span>
#include <vector>

#include "application/state/SessionRegistry.hpp"

using arkan::thanatos::application::state::SessionRegistry;

TEST(SessionRegistry, WaiterFiresOnceForNextC2S)
{
    SessionRegistry reg;

    std::atomic<int> fired{0};
    std::vector<std::uint8_t> seen;

    reg.wait_next_c2s(
        /*predicate*/ [](std::span<const std::uint8_t> b) { return b.size() == 3 && b[0] == 0xAA; },
        /*on_match*/
        [&](std::span<const std::uint8_t> b)
        {
            fired++;
            seen.assign(b.begin(), b.end());
        });

    // package that doesn't match -> ignore
    {
        const std::uint8_t bad[] = {0x00, 0x01};
        reg.notify_c2s(std::span<const std::uint8_t>(bad, sizeof(bad)));
    }
    EXPECT_EQ(fired.load(), 0);

    // first one that matches -> fires and consumes the waiter
    {
        const std::uint8_t good[] = {0xAA, 0xBB, 0xCC};
        reg.notify_c2s(std::span<const std::uint8_t>(good, sizeof(good)));
    }
    EXPECT_EQ(fired.load(), 1);
    ASSERT_EQ(seen.size(), 3u);
    EXPECT_EQ(seen[0], 0xAA);
    EXPECT_EQ(seen[1], 0xBB);
    EXPECT_EQ(seen[2], 0xCC);

    {
        const std::uint8_t again[] = {0xAA, 0xBB, 0xCC};
        reg.notify_c2s(std::span<const std::uint8_t>(again, sizeof(again)));
    }
    EXPECT_EQ(fired.load(), 1);
}
