#include "test_support.h"
#include "mordor/game_data.h"
#include "mordor/items1b.h"

using namespace mordor;

// B_0063#48: Loot.AttachesItself on the first Cursed==1 item in the data (expected false).
TEST(engine_rtt144_attaches_itself_first_cursed1_is_false) {
  const GameData& gd = tsup::data();
  const auto* found = static_cast<const ItemDef*>(nullptr);
  for (const auto& it : gd.items) {
    if (it.cursed == 1) { found = &it; break; }
  }
  CHECK(found != nullptr);
  if (found != nullptr) {
    // FIXED: attachesItself takes const Item&, but found is an ItemDef. Build a local Item
    // carrying the cursed flag from the definition. Other fields are left at their defaults.
    Item probe{};
    probe.cursed = static_cast<CurseKind>(found->cursed);
    CHECK(!attachesItself(probe));
  }
}

// SKIP B_0066#131: ItemUse.Problem returns the message text "\"held\", the Item doesn't appear to do much of anything."
// Our declarations only expose the UseCheck enum (NoEffect); no message formatter is declared to compare the string.

// SKIP B_0056#20: RechargeGuildItems not declared in our code; Character pack/guild-item fixture not declared.
// SKIP B_0056#18: RechargeGuildItems not declared in our code; Character pack/guild-item fixture not declared.
// SKIP B_0056#21: RechargeGuildItems not declared in our code; Character pack/guild-item fixture not declared.
// SKIP B_0056#16: RechargeGuildItems not declared; the expected message is a verbatim string with no declared formatter.
// SKIP B_0056#19: RechargeGuildItems not declared in our code; Character pack/guild-item fixture not declared.
// SKIP B_0056#17: RechargeGuildItems not declared; expected message is a verbatim string with a trimmed name, no declared formatter.
