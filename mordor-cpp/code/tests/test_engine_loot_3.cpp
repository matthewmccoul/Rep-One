#include "test_support.h"
#include "mordor/loot1a.h"

using namespace mordor;

// B_0057 chest find report (chestFindMonsterId).

// B_0059 gold giver tags (GiveCommand.GoldAnswer / GoldFromTag).
// The headers have no API that takes a typed string or returns a tag string;
// typedGoldWhole takes a double and giveGold takes Character/Purse arguments.

// SKIP B_0059#4: GiveCommand.GoldAnswer takes a typed string; no equivalent in headers
// SKIP B_0059#5: GiveCommand.GoldAnswer takes a typed string; no equivalent in headers
// SKIP B_0059#6: GiveCommand.GoldAnswer takes a typed string; no equivalent in headers
// SKIP B_0059#7: GiveCommand.GoldAnswer refusal text; no equivalent in headers
// SKIP B_0059#8: GiveCommand.GoldAnswer returns a tag string; no equivalent in headers
// SKIP B_0059#9: GiveCommand.GoldFromTag parses a tag string; no equivalent in headers
// SKIP B_0059#10: GiveCommand.GoldFromTag parses a tag string; no equivalent in headers
