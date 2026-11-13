// Batch t142: reference assertions from Character.SwapSlots, WearClassOf,
// WearRefusal and Effects.Recompute.
// None of the Character, SpellBuffers, Effects or item-table APIs these cases
// need are declared in our headers yet (see NEED above), so every case is
// recorded as SKIP until those declarations exist. Expected values are the
// reference ground truth and must not be altered to fit our code.

#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"

using namespace mordor;

// SKIP B_0067#15: Character.SwapSlots(3, 5, buffersFollow: false) and SpellBuffers.PackSlotOf are not declared in our headers; no Character pack or spell-buffer API to set up buffer 1 -> slot 2 and read it back. Expected: 3.

// SKIP B_0063#133: needs the MDATA3.MDR item table (GameData item field name not declared) and the maximum-worn-per-class table (maxWornOfClass not declared) to count items with a non-zero id and a name in the class whose maximum worn is -1. Expected: 7.

// SKIP B_0063#135: needs item lookup by name in MDATA3.MDR and maxWornOfClass to check that Medusa's Head is in the maximum-minus-one class. Expected: true.

// SKIP B_0063#136: needs Character construction at guild level 30 with all stats 25, Character::wearRefusal, Character::isWearable and Character::equipItem to attempt wearing the seven maximum-minus-one items. Expected: at least 2 worn.

// SKIP B_0063#22: needs Character construction, Character::equipItem for an MDATA3.MDR item whose only state bit is Levitate, Effects::recompute, and the levitating/invisibleNow/seeingInvisible/protection state fields. Expected: Lev/---/---/---.

// SKIP B_0063#23: needs Character construction, Character::equipItem for an MDATA3.MDR item whose only state bit is Invisible, Effects::recompute and the state fields. Expected: ---/Inv/---/---.

// SKIP B_0063#24: needs Character construction, Character::equipItem for an MDATA3.MDR item whose only state bit is Protect, Effects::recompute and the state fields. Expected: ---/---/---/Pro.

// SKIP B_0063#25: needs Character construction, Character::equipItem for an MDATA3.MDR item whose only state bit is See Invisible, Effects::recompute and the state fields. Expected: ---/---/See/---.
