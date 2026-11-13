#include "test_support.h"

// SKIP B_0061#185: MdrLoader.LoadDungeon and the MDATA11 area lair data have no equivalent in the headers.
// SKIP B_0061#186: Name comparison against the page's lair list needs MdrLoader.LoadDungeon and page data; no equivalent.
// SKIP B_0061#187: Companion has no CompanionId field, and MdrLoader.LoadMonsters is not in the headers.
// SKIP B_0061#188: Requires MdrLoader.LoadDungeon to map area lair monsters; no equivalent in the headers.
// SKIP B_0061#189: Rare-monster list comparison needs the page's rarity table and MdrLoader.LoadMonsters; no equivalent.
// SKIP B_0061#190: Rare-monster list row comparison needs the page's rarity table and MdrLoader.LoadMonsters; no equivalent.
// SKIP B_0061#191: Counting living monsters by rarity needs MdrLoader.LoadMonsters on MDATA5; no equivalent.
// SKIP B_0062#5: Counting shared monster ids needs MdrLoader.LoadMonsters and GroupBy on MDATA5.MDR; no equivalent.
// SKIP B_0062#6: Finding the duplicated monster id needs MdrLoader.LoadMonsters and GroupBy; no equivalent.
// SKIP B_0062#7: Building the name and level string needs MdrLoader.LoadMonsters on MDATA5; no equivalent.
