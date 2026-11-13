#include "test_support.h"
#include "mordor/state_common.h"
#include "mordor/items1a.h"

using namespace mordor;

// NEED: InventorySlot (Known, Alignment, Charges fields), Character (Name, Stats, StatWithMods, TopGuildLevel), Item (Floor, LevelScale, Id), a gear lookup type for Func<short, Item*>, and our ReadGiven equivalent (the handover that sets slot.Known and files the item).

// SKIP B_0064#18: the assertion depends on the handed-over setup from the same reference case (the slot's Known after a ReadGiven-style handover, with a gear lookup resolving LevelScale). Our code has readableLevel(Ctx&, Character, Item) and no handover function or InventorySlot in the declarations provided, and the lookup-based ReadableLevel overload is not declared, so the scenario cannot be reproduced faithfully. Needs the handover function and InventorySlot declared, plus the sibling case setup.
