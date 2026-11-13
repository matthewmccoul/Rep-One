#include "test_support.h"
#include "mordor/monsters1a.h"

// Case ids contain '#', which is not valid in a C++ identifier, so '#' is written as '_' in test names.

// SKIP B_0055#18: needs MDATA5.MDR loader (MdrLoader.LoadMonsters); no loader function in headers
// SKIP B_0055#36: needs MDATA5.MDR loader and a duplicate-Id scan; no equivalent function in headers
// SKIP B_0055#93: CombatEngine.GoldToSteal has no equivalent function in headers
// SKIP B_0055#94: CombatEngine.GoldToSteal has no equivalent function in headers
// SKIP B_0055#95: CombatEngine.GoldToSteal has no equivalent function in headers
// SKIP B_0055#140: CombatEngine.GoldToSteal has no equivalent function in headers
// SKIP B_0056#1: CombatMonster.LoadRecord needs MDATA5 records and combat records; no equivalent in headers
// SKIP B_0056#3: EncounterGenerator.InRoom/Capture need MDATA11 dungeon data and room capture; no equivalent in headers
// SKIP B_0056#4: EncounterGenerator.InRoom store/read-back needs room persistence; no equivalent in headers

