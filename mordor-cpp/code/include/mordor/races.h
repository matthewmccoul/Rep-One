#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"

namespace mordor {

// ---- Constants and enums (Chapter 3: Races, part c03p1) ----

// R3 fixed offset above race max stat = play ceiling
inline constexpr i32 kStatCeilingOffset = 5;

// R9/R10 number of playable races (Table T1 order: Human..Troll)
inline constexpr i32 kRaceCount = 9;

// R10 Exp approximation: log base (Human Exp 3 gives multiplier 1.0)
inline constexpr f64 kExpLogBase = 3.0;
// R10 Exp approximation: constant in denominator
inline constexpr f64 kExpDenomConst = 4.50;
// R10 Exp approximation: offset subtracted from Exp in denominator
inline constexpr f64 kExpDenomOffset = 4.0;
// R10 Exp approximation: slope constant in denominator (11/90)
inline constexpr f64 kExpDenomSlope = 11.0 / 90.0;

// R7 resistance index order: Fire, Cold, Electrical, Mind, Disease, Poison,
// Magic, Stone, Paralysis, Drain, Acid. Values index Resistances.resist.
enum class ResistCategory : i32 {
    Fire = 0,
    Cold,
    Electrical,
    Mind,
    Disease,
    Poison,
    Magic,
    Stone,
    Paralysis,
    Drain,
    Acid
};

// R11 archive racial penalty normalized to Human = 1.0, indexed by static_cast<i32>(Race).
// Table T5 order: Human, Elf, Giant, Gnome, Dwarf, Ogre, Morloch, Osiri, Troll.
// CONFLICT: archive [c103] rounds these (1.188, 1.105, ...); exp-derived [c97] values are
// more precise. Prefer [c97] values here as stated by the SRD.
inline constexpr f64 kRacialPenalty[kRaceCount] = {
    1.00000000000,
    1.18544741085,
    1.18544741085,
    1.18544741085,
    1.10551994263,
    1.14714602440,
    1.05819132268,
    1.22165868435,
    1.25647909518
};

// ---- R2 base stats ----


// ---- R3 stat ceiling and raise ----

// R3 highest value a stat may reach in play: race max for stat + kStatCeilingOffset.
// StatIndex order is Str, Int, Wis, Con, Cha, Dex (Table T2 order).
// CONFLICT: types.h declares a Stat enum; state_character.h StatIndex is used here
// because Character.abilities is indexed by it.
i32 statCeiling(Ctx& ctx, Race race, StatIndex stat);


// ---- R4 guild eligibility ----

// R4 race-guild eligibility from the Table T4 grid.
// UNKNOWN: whether the engine enforces the grid. Trolls are barred from many guilds (R14);
// exact list UNKNOWN, grid data is authoritative.
bool guildAllowed(Ctx& ctx, Race race, Guild guild);

// ---- R5 alignment ----

// R5 alignment must lie inside the race alignment mask (Table T1).
// UNKNOWN: enforcement point (character creation vs later changes).
bool alignmentAllowed(Ctx& ctx, Race race, Align align);

// ---- R6 / R7 resistances ----



// ---- R8 age ----


// ---- R9 / R10 / R11 experience ----

// R9 XP required for ch.level, scaled by the racial multiplier.
// UNKNOWN: the XP formula body (uses r and raceFactor per [c88, c103]).
i64 xpRequired(Ctx& ctx, const Character& ch);


// R11 archive racial penalty normalized to Human = 1.0 (Table T5).
// Pure table lookup; see kRacialPenalty for the CONFLICT note.
f64 racialPenalty(Race race);

// ---- R12 item use ----


// ---- R13 charm ----


}  // namespace mordor
