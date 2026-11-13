#include "test_support.h"

// Batch t094. Every case in this batch targets a function whose C++ declaration is not
// among the shown declarations, so none can be expressed yet. Each is skipped with the
// reason, and the NEED line below lists what is missing.

using namespace mordor;

// SKIP B_0061#42: needs GuildData.ByName("Wizard").SpellRate; no C++ guild lookup or GameData guild field names are declared (guild table field names not shown)
// SKIP B_0061#58: needs Rules.Fn1485Price(1, 110, 1.0f, 1); no C++ fn1485Price declaration is shown
// SKIP B_0061#59: needs Rules.Fn1485Price(1, 100, 1.1f, 1) with single-precision 1.1f; no C++ fn1485Price declaration is shown
// SKIP B_0061#60: needs Rules.Fn1485Price(1, 0, 1.0f, 1); no C++ fn1485Price declaration is shown
// SKIP B_0061#107: needs Rules.Fn1485Price(6, 40, 1.25f, 20) and its out parameter for the rated level; no C++ fn1485Price declaration is shown
// SKIP B_0066#24: needs SpellBuffers.CostText for a slot past the end of the MDATA2.MDR spell table; no C++ SpellBuffers type or spell table in GameData is declared
// SKIP B_0066#26: needs SpellBuffers.CostText returning "?" for a spell no guild prices; no C++ SpellBuffers type or Spellbook cost function is declared
// SKIP B_0066#22: needs SpellBuffers.Describe for slot 0 past the spell table; no C++ SpellBuffers type is declared
