#include "test_support.h"
#include "mordor/loot1b.h"
#include "mordor/game_data.h"
#include "mordor/items1b.h"

using namespace mordor;

// SKIP B_0066#131: ItemUse.Problem returns the message text "\"held\", the Item doesn't appear to do much of anything."
// Our declarations only expose the UseCheck enum (NoEffect); no message formatter is declared to compare the string.

// SKIP B_0056#20: RechargeGuildItems not declared in our code; Character pack/guild-item fixture not declared.
// SKIP B_0056#18: RechargeGuildItems not declared in our code; Character pack/guild-item fixture not declared.
// SKIP B_0056#21: RechargeGuildItems not declared in our code; Character pack/guild-item fixture not declared.
// SKIP B_0056#16: RechargeGuildItems not declared; the expected message is a verbatim string with no declared formatter.
// SKIP B_0056#19: RechargeGuildItems not declared in our code; Character pack/guild-item fixture not declared.
// SKIP B_0056#17: RechargeGuildItems not declared; expected message is a verbatim string with a trimmed name, no declared formatter.
