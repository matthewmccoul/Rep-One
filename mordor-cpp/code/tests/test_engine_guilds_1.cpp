#include "test_support.h"
#include "mordor/guilds1.h"
#include "mordor/guilds3.h"

// SKIP B_0059#52: asks about the structure of the original FRMCTYG.BAS source, not runtime behaviour; no equivalent function
// SKIP B_0059#53: asks about the original FRMCTYG.BAS loop caps and VB And short-circuiting; not runtime behaviour
// SKIP B_0059#54: needs an MDATA5 monster-class fixture (class/monster/Chance layout), which the headers do not expose
// SKIP B_0059#55: needs an MDATA5 monster-class fixture and a draw counter; no way to observe draws from the headers
// SKIP B_0059#56: needs an MDATA5 monster-class fixture; questMonster(ctx, tier) depends on data layout not given here
// SKIP B_0059#57: needs an MDATA5 monster-class fixture; questMonster(ctx, tier) depends on data layout not given here
// SKIP B_0059#58: needs an MDATA5 monster-class fixture and a draw counter
// SKIP B_0059#60: needs an MDATA3 item-run fixture and a draw counter for QuestItem
// The pick step below is the only part of the item-run case that takes indices directly, so it needs no data fixture.
// Run bounds (band, price, curse, rarity) are checked by the data loader, not by PickItemIndex.

