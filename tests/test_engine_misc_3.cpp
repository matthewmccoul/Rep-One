#include "test_support.h"
#include "mordor/misc.h"
#include "mordor/state_game.h"

// B_0064#32: the Messages window's default line count is the Options default.
TEST(engine_B_0064_32_messages_window_keeps_20_lines) {
    mordor::Options opts{};
    CHECK_EQ(opts.messagesKept, 20);
}

// SKIP B_0062#75: no TryCLng equivalent in headers (string parsing of "1E3")
// SKIP B_0062#76: no TryCLng equivalent in headers (string parsing of "2.5D2")
// SKIP B_0062#77: no TryCLng equivalent in headers; roundHalfEven is only the rounding step, not the call under test
// SKIP B_0062#78: no TryCLng equivalent in headers; roundHalfEven is only the rounding step, not the call under test
// SKIP B_0062#79: no TryCLng equivalent in headers; roundHalfEven is only the rounding step, not the call under test
// SKIP B_0062#80: no TryCLng equivalent in headers (string parsing and refusal of "abc")
// SKIP B_0062#81: no TryCLng equivalent in headers (Long range refusal)
// SKIP B_0064#33: no MessageList.Add equivalent in headers; needs a message list simulation
// SKIP B_0064#34: no MessageList.Add equivalent in headers; needs a message list simulation
