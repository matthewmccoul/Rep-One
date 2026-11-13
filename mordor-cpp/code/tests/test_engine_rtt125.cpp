#include "test_support.h"
// NEED: EncounterGenerator (Generate, InRoom), Encounter (Count, TreasureClass, Fresh, Failed, Room, ToRecord, Take), CombatMonster (Hits, MaxHits, KnownAs, Ref, Alive, AtNought, Unrolled, Runs), DungeonState (LoadRoom, StoreRoom, RefillDue, NoteRefilled), Floor (AreaIdAt, HeaderAreaCount), Monster, Character
using namespace mordor;

// No encounter, combat-group, or dungeon-room declarations exist in our headers yet,
// so none of these reference cases can be expressed against our API.

// SKIP B_0056#9: needs EncounterGenerator.Generate, CombatMonster.Runs/Unrolled (no declarations in our code)
// SKIP B_0066#107: needs EncounterGenerator.InRoom and CombatMonster.Hits (no declarations in our code)
// SKIP B_0066#108: needs EncounterGenerator.InRoom and CombatMonster.MaxHits (no declarations in our code)
// SKIP B_0066#109: needs EncounterGenerator.InRoom and group count on CombatMonster (no declarations in our code)
// SKIP B_0066#110: needs EncounterGenerator.InRoom and CombatMonster.KnownAs (no declarations in our code)
// SKIP B_0066#111: needs EncounterGenerator.InRoom to detect generation of a new encounter (no declarations in our code)
// SKIP B_0066#106: needs EncounterGenerator.InRoom, DungeonState.StoreRoom, and CombatMonster.Ref (no declarations in our code)
// SKIP B_0056#4: needs EncounterGenerator.InRoom, DungeonState.StoreRoom, CombatMonster.Alive/AtNought/Hits/Unrolled (no declarations in our code)
