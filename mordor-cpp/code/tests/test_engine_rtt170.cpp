// Batch t170: translation of Confinement / AlignmentChoice / StoreStock assertions.
// None of the Confinement, AlignmentChoice, ItemAlignment or Monster declarations are in
// our headers yet (see NEED above), so every case is skipped for now. Once those are declared,
// the Confinement cases (B_0055#23, #25, #10, #47, #48) can be written against our API with
// the expected values below unchanged.
#include "test_support.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"

using namespace mordor;

// SKIP B_0055#9: needs AlignmentChoice.For and Confinement.Add/Count, plus the first monster in MDATA5.MDR (MdrLoader.LoadMonsters); no monster loader or AlignmentChoice in our headers.
// SKIP B_0055#48: needs Confinement.Add/Take/Count and a hand-made Monster with no table index; Confinement and Monster are not declared in our headers.
// SKIP B_0055#23: needs Confinement.Count keyed by table index and alignment, with Morloch and monsters[1] pens; Confinement and the monster table are not declared in our headers.
// SKIP B_0055#25: needs Confinement.Count after the repair step for monsters[1]; Confinement is not declared in our headers.
// SKIP B_0055#10: needs Confinement.PriceToBuy and Confinement.Add; Confinement is not declared in our headers.
// SKIP B_0055#47: needs Confinement.PriceToBuy, Confinement.BaseValue and the Morgul and Shadow Ninja monsters from MDATA5.MDR; not declared in our headers and the data loader is missing.
// SKIP B_0063#63: needs Confinement.Replenish on a 6000-run pen restock with deepestReached 15, which needs the full monster table and a run simulator; not declared in our headers.
// SKIP B_0063#64: needs Confinement.Replenish on the same 6000-run restock and reads the deepest LevelFound of penned creatures; not declared in our headers.
