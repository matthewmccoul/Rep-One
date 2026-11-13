#include "test_support.h"

using namespace mordor;

// SKIP B_0061#89: CombatOrder::CheckCastClearsAtt / ClearAtt not declared in our headers (NEED: CombatOrder, checkCastClearsAtt)
// SKIP B_0058#30: C# UI source question (DungeonWindow.axaml.cs draw order), not expressible as a unit test of our API
// SKIP B_0058#32: C# UI source question (DungeonWindow.Mdi.cs SheetClassBox.SelectionChanged), not expressible as a unit test of our API
// SKIP B_0058#33: C# UI source question (DungeonWindow.Mdi.cs sheet reopen), not expressible as a unit test of our API
// SKIP B_0058#31: C# UI source question (DungeonWindow.Mdi.cs scroll gesture), not expressible as a unit test of our API
// SKIP B_0062#59: needs MDATA2 spell records plus per-guild SpellMax ceilings; no GameData spell or guild-ceiling fields declared (NEED: GameData spell table, guild SpellMax)
// SKIP B_0061#34: needs GuildData.ByName("Nomad").ShownSpellLevel equivalent; not declared (NEED: shownSpellLevel, guild lookup)
// SKIP B_0061#33: needs GuildData.ByName("Thief").ShownSpellLevel equivalent; not declared (NEED: shownSpellLevel, guild lookup)
