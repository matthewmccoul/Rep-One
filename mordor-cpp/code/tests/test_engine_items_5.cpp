#include "test_support.h"

// SKIP B_0062#68: Needs MDATA3.MDR loaded and an item looked up by name (Medusa's Head). No loader or name lookup is in the headers, so the item cannot be constructed.
// SKIP B_0062#69: Scans every MDATA3 item for a wear-class maximum of 0. Needs the MDATA3 loader and a whole-table scan, which the headers do not provide.
// SKIP B_0062#70: Needs a consumable item whose wear class resolves to 0 from data. Item type codes 23..28 are not representable in the ItemType enum, and wearClassOf depends on ItemDef data that cannot be set up here.
// SKIP B_0063#22: Requires an MDATA3 item whose only state bit is Levitate. The reference's Effects.Recompute has no header equivalent, and the item fixture cannot be built from the headers.
// SKIP B_0063#23: Requires an MDATA3 item whose only state bit is Invisible. Same reason as B_0063#22.
// SKIP B_0063#24: Requires an MDATA3 item whose only state bit is Protect. Same reason as B_0063#22.
// SKIP B_0063#25: Requires an MDATA3 item whose only state bit is See Invisible. Same reason as B_0063#22.
// SKIP B_0063#43: Counts MDATA3 items by id, cursed value and floor. This is a whole-table scan that needs the MDATA3 loader.
// SKIP B_0063#44: Tests every findable cursed MDATA3 item for refusal over a full character setup. Needs the MDATA3 data set and a simulation of the equip/refusal flow.
// SKIP B_0063#45: Counts MDATA3 items with Cursed == 1. This is a whole-table scan that needs the MDATA3 loader.
