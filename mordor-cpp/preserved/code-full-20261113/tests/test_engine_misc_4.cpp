#include "test_support.h"

// SKIP B_0064#35: MessageList is not in the headers; the only message-related state is GameState::messages and Options::messagesKept, with no Add function.
// SKIP B_0064#36: no MessageList.Add equivalent in the headers; blank-line filtering cannot be exercised.
// SKIP B_0064#37: no MessageList.Add equivalent in the headers; duplicate-line acceptance cannot be exercised.
// SKIP B_0064#38: no MessageList newest/second-newest accessor in the headers; the joined-text view cannot be produced.
// SKIP B_0064#39: no MessageList count accessor in the headers; kMessagesKept only gives the default capacity, not a held count.
// SKIP B_0065#115: the party-blow quest completion message has no function in the headers; it needs a combat/quest message path.
// SKIP B_0065#116: the Fight-order quest message needs a combat round simulation, which the headers do not expose as a callable function.
// SKIP B_0065#117: no play-clock tick function in the headers; GameState::playElapsedMs and Character::playSeconds are state fields only.
// SKIP B_0065#118: no play-clock tick function in the headers; the fractional-second tick cannot be driven through an API.
// SKIP B_0065#119: no play-clock tick function in the headers; the day-wrap behaviour (86340-second day) has no equivalent function.
