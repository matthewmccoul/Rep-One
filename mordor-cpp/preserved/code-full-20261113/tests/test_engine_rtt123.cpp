// NEED: CombatMonster (LoadRecord, TakeRecord, Runs, Unrolled, Hits, MaxHits, Group, Believed, Alive, Hostile, JoinOffer, Identified, AtNought), RoomRecord (Foes, Copy, TreasureClass, ChestTrap, ChestLock, Gold), FoeSlot (GroupCount, KnownAs, Hits, MaxHits, Engaged, JoinOffer, IdLevel), CombatEngine (ChestClass, ChestTrap, ChestLocked, GoldEarned), EncounterGenerator (Capture, InRoom), GameData monster table with Id, and a draw counter on ScriptedRng (reference ScriptedRandom.Draws)

#include "test_support.h"

using namespace mordor;

// SKIP B_0056#12: needs CombatMonster::LoadRecord, Unrolled and a monster table with Id/hits; none declared in our headers
// SKIP B_0056#11: needs CombatMonster::Runs and the unrolled/stored front state; not declared in our headers
// SKIP B_0066#99: needs EncounterGenerator::Capture, RoomRecord/FoeSlot and CombatMonster JoinOffer; not declared in our headers
// SKIP B_0057#28: needs EncounterGenerator::Capture, RoomRecord/FoeSlot GroupCount/KnownAs and CombatMonster Believed; not declared in our headers
// SKIP B_0057#29: needs EncounterGenerator::Capture and RoomRecord/FoeSlot KnownAs readback; not declared in our headers
// SKIP B_0057#31: needs EncounterGenerator::Capture and CombatMonster Believed/KnownAs; not declared in our headers
// SKIP B_0056#8: needs EncounterGenerator::Capture over a multi-round combat sequence (group-1 felling order); CombatEngine and the round loop are not declared in our headers
// SKIP B_0056#3: needs EncounterGenerator::InRoom on MDATA11 dungeon 2 at (6,5) plus Capture; no dungeon generator or encounter API declared in our headers
