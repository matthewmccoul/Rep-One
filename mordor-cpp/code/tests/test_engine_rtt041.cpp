// NEED: ScriptedRng::draws (draw counter on the scripted RNG, used by the draw-count cases)
// NEED: GameData monster/item record field names (tier, floor, price, cursed, rarity, type) and a way to build a custom item/monster table for the run-shape cases
#include "test_support.h"
#include "mordor/guilds1.h"

using namespace mordor;

// SKIP B_0061#127: needs the community A/D tables (Rules.GuildTables), which our API does not expose; cannot compare every published value.
// SKIP B_0061#128: needs the count of entries in the community A/D tables; GuildTables is not exposed in our API.
// SKIP B_0059#60: needs the MDATA3 item fixture (kind-centre roll depends on the item-type count and table layout) and a draw counter on ScriptedRng that our API does not show.
// SKIP B_0059#62: needs a one-item run fixture in MDATA3 with known record fields; GameData item field names are not declared.
// SKIP B_0059#66: our questItemReachable(Ctx&, tier) takes a tier and reads the item table from ctx; it cannot be given band 7 with zero item kinds.
// SKIP B_0059#65: our questItemReachable takes a tier, not a band, and the tier-to-band mapping is not part of the shown API; cannot express the band 1-7 sweep faithfully.
// SKIP B_0059#55: needs the monster fixture for tier 2 and a draw counter on ScriptedRng that our API does not show.
// SKIP B_0059#56: needs the monster fixture (first monster at tier 2 is data-dependent) and monster record field names, which are not declared.

