#include "test_support.h"

using namespace mordor;

// Batch t173: StoreStock.Replenish / Confinement.Replenish / Weight reference cases.
// No tests yet: the store, pen and library API is not declared in our headers.

// SKIP B_0057#104: StoreStock.Replenish not declared; needs items with Cursed/Floor and a shelf query
// SKIP B_0057#105: StoreStock.Replenish not declared; needs a seed loop (1..199) over Replenish and a shelf count
// SKIP B_0057#106: StoreStock.Replenish and Librarian.FileRestock not declared; needs a shelved callback and shelf comparison
// SKIP B_0059#47: StoreStock.Replenish not declared; needs ScriptedRng-driven pass/sell/floor/kind/pick/alignment draws and a shelf count
// SKIP B_0059#49: StoreStock.Replenish not declared; needs ScriptedRng with 150 non-matching kind draws and a shelf count
// SKIP B_0059#51: StoreStock.Replenish not declared; needs a draw counter on ScriptedRng (not in our test support)
// SKIP B_0067#14: StoreStock.Replenish and Mdata1 item list not declared; needs MDATA3 item load and cursed-count query
// SKIP B_0059#44: StoreStock.Weight not declared; needs the Weight(rarity, total) function and its integer return type
