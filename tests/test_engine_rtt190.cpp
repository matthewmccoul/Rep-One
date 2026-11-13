#include "test_support.h"

// Batch t190: reference assertions from Character.LoadAllFrom, CityWindow,
// CityWindow.ShowDead and CombatEngine.MonsterAttacks.
// None of these can be expressed against the declarations available so far,
// so every case is skipped with its reason. Expected values are not altered.

using namespace mordor;

// SKIP B_0059#93: LoadAllFrom reads save files from a folder; core never reads files, so the order check belongs to the port's file layer, not to a core test.
// SKIP B_0058#16: ArriveDeadSub0930 is a CityWindow (Avalonia UI) method; no core equivalent is declared in our headers yet (NEED: ArriveDeadSub0930/PruneTheDead rule).
// SKIP B_0058#4: ShowControlPanel ordering (ShowDialog then ArriveDeadSub0930) is UI control flow in CityWindow.axaml.cs; not expressible in core.
// SKIP B_0068#10: ShowDead() button visibility is UI screen logic in CityWindow; no declared rule for the Hire Rescuers button (NEED: dead-screen button rule).
// SKIP B_0068#11: ShowDead() button visibility is UI screen logic in CityWindow; no declared rule for the Visit the Morgue button (NEED: dead-screen button rule).
// SKIP B_0068#12: ShowDead() button visibility is UI screen logic in CityWindow; no declared rule for death-cause-based buttons (NEED: dead-screen button rule).
// SKIP B_0066#136: needs a Character/Party with hits, paralysis and a monster group plus the CombatEngine.MonsterAttacks entry point; none declared (NEED: Character, Party, MonsterAttacks signature).
// SKIP B_0066#138: needs the same combat entry point and the "You were killed by a X." message from the death path (NEED: Dies/SetDead rule and CombatLine output).
