#include "test_support.h"

// SKIP B_0066#110: needs a walk-out simulation over a return visit; no headers expose a walk-out or KnownAs-after-visit function.
// SKIP B_0066#111: needs an encounter-generation simulation across a return visit; no equivalent function in the headers.
// SKIP B_0066#140: needs a count over every MDATA5.MDR monster for companion class/index range; no headers expose that check or an MDATA5 loader.
// SKIP B_0066#141: needs building encounter state tables from MDATA5.MDR and detecting unreachable like-types states; no equivalent function in the headers.
// SKIP B_0066#142: needs building encounter state tables from MDATA5.MDR and detecting restart states with no exit; no equivalent function in the headers.
// SKIP A_03#29: needs the MDATA15 pen-record importer; no importer appears in the headers.
// SKIP A_03#35: needs the MDATA13 library-record importer; no importer appears in the headers.
// SKIP A_03#45: needs Confinement.Load and the import pipeline; no equivalent in the headers.
// SKIP A_03#47: needs Librarian.Store.Load and ForMonster; no equivalent in the headers (libraryKnowledgeFor maps a state value, not a store).
