// items3b.cpp: item spawn, identification, guild use, charge items, stat mods, thieving.
// Implements include/mordor/items3b.h. SRD rules R1-R18 and formulas F1-F6 (Part C).
//
// BUILD STATUS: NOT YET COMPILING. The ItemDef members 'category', 'level', 'chance' and
// 'timesCast' do not exist in ItemDef (game_data.h, not supplied). Their correct names must be
// confirmed from that header before these lines can compile. No names were guessed.
//
// ASSUMED (not supplied in the brief; reconcile with game_data.h, events.h, rng.h):
//   ItemDef fields: level (i32), category (i32), chance (double), damageMod (double),
//     levelScale (double; used as an item index in R13), statMods (Arr<i32,kStatCount>),
//     spellId (i32), spellLevel (i32), timesCast (i32).
//   GameData::items is an indexable Vec<ItemDef>; an item's id is its index in that Vec.
//   IEventSink nums are Vec<i64>; strs are Vec<Str>.
//   IRng::below(n) returns an integer in [0,n); IRng::nextDouble() returns [0,1).
//   Abilities::stats holds natural stats; Abilities::statMods holds modifier stats.

#include <cmath>

#include "mordor/items3b.h"

namespace mordor {
namespace {

// R11: "dungeon level 1-5" for the MOSTLY band. Lower bound.
constexpr i32 kMostlyMinDungeonLevel = 1;

}  // namespace

}  // namespace mordor
