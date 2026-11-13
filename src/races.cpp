#include "mordor/races.h"

#include <cmath>

#include "mordor/math_util.h"

namespace mordor {
namespace {

// Race index order follows the Race enum: Human, Elf, Giant, Gnome, Dwarf, Ogre, Morloch, Osiri, Troll.
// Stat order follows StatIndex: Str, Int, Wis, Con, Cha, Dex.
// UNKNOWN: GameData.races field names were not supplied, so the SRD tables are transcribed here
// (T1, T2, T3, T4 of c03p1). If RaceDef is available, these should be read from ctx.data instead.

// T2. Stat min bounds [c62, c87].
constexpr i32 kStatMinTable[kRaceCount][kStatCount] = {
    {4, 4, 4, 6, 5, 6},    // Human
    {3, 7, 7, 3, 3, 3},    // Elf
    {12, 4, 3, 9, 2, 3},   // Giant
    {4, 7, 5, 3, 9, 6},    // Gnome
    {3, 3, 7, 3, 3, 5},    // Dwarf
    {9, 3, 3, 9, 3, 5},    // Ogre
    {6, 3, 5, 3, 2, 5},    // Morloch
    {5, 3, 3, 7, 3, 10},   // Osiri
    {6, 3, 3, 6, 3, 6},    // Troll
};

// T2. Stat max bounds, also the M553C race max used by R3 [c62, c87].
constexpr i32 kStatMaxTable[kRaceCount][kStatCount] = {
    {17, 18, 18, 17, 18, 18},  // Human
    {15, 20, 20, 16, 18, 18},  // Elf
    {25, 17, 17, 19, 16, 18},  // Giant
    {17, 19, 19, 17, 22, 18},  // Gnome
    {18, 18, 19, 19, 17, 18},  // Dwarf
    {20, 16, 16, 21, 18, 17},  // Ogre
    {20, 17, 19, 15, 14, 20},  // Morloch
    {17, 18, 17, 19, 18, 22},  // Osiri
    {20, 18, 18, 19, 17, 20},  // Troll
};

// T4. Race-guild eligibility grid, true = may join [c71].
// Column order follows the Guild enum: Nomad, Warrior, Paladin, Ninja, Villain, Seeker,
// Thief, Scavenger, Mage, Sorcerer, Wizard, Healer.
constexpr bool kGuildGrid[kRaceCount][kGuildCount] = {
    // Human: all 12 [c71, c96]
    {true, true, true, true, true, true, true, true, true, true, true, true},
    // Elf: PAL, SEE, THI, SCA, MAG, SOR, WIZ, HEA
    {false, false, true, false, false, true, true, true, true, true, true, true},
    // Giant: NOM, WAR, SEE
    {true, true, false, false, false, true, false, false, false, false, false, false},
    // Gnome: PAL, VIL, SEE, THI, SCA, MAG, SOR, HEA
    {false, false, true, false, true, true, true, true, true, true, false, true},
    // Dwarf: NOM, WAR, SEE, THI, SCA, SOR, WIZ, HEA
    // CONFLICT: [c96] says every guild; grid [c71] used per stated preference.
    {true, true, false, false, false, true, true, true, false, true, true, true},
    // Ogre: NOM, WAR, SCA
    {true, true, false, false, false, false, false, true, false, false, false, false},
    // Morloch: NOM, WAR, VIL, SEE, SCA, MAG, SOR
    {true, true, false, false, true, true, false, true, true, true, false, false},
    // Osiri: NOM, SEE, THI, SCA, SOR, WIZ, HEA
    {true, false, false, false, false, true, true, true, false, true, true, true},
    // Troll: NOM, NIN, VIL, THI, SCA
    {true, false, false, true, true, false, true, true, false, false, false, false},
};

// T1 + section 3 alignment mask, columns Good, Neutral, Evil [c62, c71].
// CONFLICT: wiki [c96, c87] gives different masks for Giant, Ogre, Dwarf, Morloch, Osiri, Troll.
// Preferred [c62, c71] values are used.
constexpr bool kAlignMask[kRaceCount][3] = {
    {true, true, true},    // Human
    {true, true, true},    // Elf
    {false, true, false},  // Giant
    {true, true, true},    // Gnome
    {false, true, false},  // Dwarf
    {true, true, true},    // Ogre
    {true, false, true},   // Morloch
    {false, true, false},  // Osiri
    {true, true, true},    // Troll
};

// T3. Resistances in percent, order R7: Fire, Cold, Electrical, Mind, Disease, Poison,
// Magic, Stone, Paralysis, Drain, Acid [c62, c82].
// CONFLICT: Morloch Cold is 35 here [c62, c82]; [c87] says 53 (likely typo).
// CONFLICT: Morloch Disease/Poison are 0 here [c62]; [c83] says resistant.
constexpr i32 kResistTable[kRaceCount][kResistKinds] = {
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},          // Human
    {0, 0, 0, 50, 0, 0, 80, 0, 0, 0, 0},        // Elf
    {50, 50, 50, 0, 50, 50, 0, 25, 50, 50, 0},  // Giant
    {0, 0, 0, 40, 0, 0, 0, 0, 0, 0, 0},         // Gnome
    {45, 80, 0, 0, 50, 50, 0, 40, 50, 90, 0},   // Dwarf
    {75, 75, 0, 0, 50, 0, 0, 50, 75, 75, 15},   // Ogre
    {35, 35, 35, 50, 0, 0, 40, 0, 95, 65, 50},  // Morloch
    {0, 0, 90, 90, 0, 0, 0, 90, 0, 0, 90},      // Osiri
    {0, 0, 0, 0, 45, 45, 0, 0, 45, 65, 25},     // Troll
};

// T1 Exp factor per race [c62, c97].
constexpr i32 kExpTable[kRaceCount] = {3, 7, 7, 7, 5, 6, 4, 8, 9};

i32 raceIndex(Race race) {
    return static_cast<i32>(race);
}

}  // namespace

// R2: base stat is drawn uniformly within the race min-max range.
// UNKNOWN: the roll procedure is not in the source; a uniform draw over [min, max] is used.
i32 generateBaseStat(Ctx& ctx, Race race, StatIndex stat) {
    const i32 r = raceIndex(race);
    const i32 s = static_cast<i32>(stat);
    const i32 lo = kStatMinTable[r][s];
    const i32 hi = kStatMaxTable[r][s];
    return lo + static_cast<i32>(ctx.rng.below(hi - lo + 1));
}

// R3: ceiling = Races(r).M553C(stat) + 5, where M553C is the race max stat.
i32 statCeiling(Ctx& ctx, Race race, StatIndex stat) {
    (void)ctx;
    return kStatMaxTable[raceIndex(race)][static_cast<i32>(stat)] + kStatCeilingOffset;
}

// R3: a stat may be raised only up to the ceiling. Amount is added and clamped to the ceiling.
// UNKNOWN: whether statMax is also updated on a raise is not in the source; it is left unchanged.
i32 raiseStat(Ctx& ctx, Character& ch, StatIndex stat, i32 amount) {
    const u32 idx = static_cast<u32>(stat);
    const i32 current = ch.abilities.stats[idx];
    const i32 ceiling = statCeiling(ctx, ch.race, stat);
    const i32 wanted = current + amount;
    const i32 target = wanted < ceiling ? wanted : ceiling;
    if (target > current) {
        ch.abilities.stats[idx] = target;
    }
    return ch.abilities.stats[idx];
}

// R4: race-guild eligibility from grid T4.
// UNKNOWN: whether the engine enforces the grid; the grid is returned as data.
bool guildAllowed(Ctx& ctx, Race race, Guild guild) {
    (void)ctx;
    return kGuildGrid[raceIndex(race)][static_cast<i32>(guild)];
}

// R5: alignment must lie inside the race's alignment mask.
// UNKNOWN: enforcement point; the mask is returned as data.
bool alignmentAllowed(Ctx& ctx, Race race, Align align) {
    (void)ctx;
    return kAlignMask[raceIndex(race)][static_cast<i32>(align)];
}

// R6: resistance percentage for a category. Categories not listed are 0.
i32 resistancePercent(Ctx& ctx, Race race, ResistCategory category) {
    (void)ctx;
    return kResistTable[raceIndex(race)][static_cast<i32>(category)];
}

// R6: percentage applied against an amount.
// UNKNOWN: order and stacking with spell/item resistance are not in the source.
// Simplest choice: reduce the amount by pct percent, rounded half-even.
i32 applyResistance(Ctx& ctx, Race race, ResistCategory category, i32 amount) {
    const i32 pct = resistancePercent(ctx, race, category);
    const i64 reduction = roundHalfEven(static_cast<f64>(amount) * static_cast<f64>(pct) / 100.0);
    return amount - static_cast<i32>(reduction);
}

// R8: age runs toward the race max age.
// UNKNOWN: aging mechanics and units of ageDays are not in the source.
// Simplest choice: advance ageDays by one unit per call.
void advanceAge(Ctx& ctx, Character& ch) {
    (void)ctx;
    ch.ageDays += 1;
}

// R9: XP required is scaled by the racial multiplier.
// UNKNOWN: the XP formula body is not in the source. Simplest choice: scale the character's
// xpRequired field by the R11 racial penalty r, rounded half-even.
i64 xpRequired(Ctx& ctx, const Character& ch) {
    (void)ctx;
    const f64 scaled = static_cast<f64>(ch.xpRequired) * racialPenalty(ch.race);
    return roundHalfEven(scaled);
}

// R10: approximate XP multiplier derived from Exp.
// Actual Exp approx = 1 + [ln(Exp)/ln(3) - 1] / [4.50 - (Exp - 4) * (11/90)]
// CONFLICT: this formula does not reproduce the table in [c97] for Elf, Osiri and Troll
// (hand recomputation gives about 1.1866, 1.2226, 1.2571). The formula is implemented as written.
f64 expMultiplier(Ctx& ctx, Race race) {
    (void)ctx;
    const f64 exp = static_cast<f64>(kExpTable[raceIndex(race)]);
    const f64 num = std::log(exp) / std::log(kExpLogBase) - 1.0;
    const f64 den = kExpDenomConst - (exp - kExpDenomOffset) * kExpDenomSlope;
    return 1.0 + num / den;
}

// R11: normalized racial penalty r, Human = 1.0, from table T5 (kRacialPenalty in header).
f64 racialPenalty(Race race) {
    return kRacialPenalty[raceIndex(race)];
}

// R12: Elves can never use items that require great Strength [c87].
// UNKNOWN: the item strength requirement is not in Item or the supplied GameData fields.
// Until it is available, no item is treated as requiring great Strength.
namespace {
bool requiresGreatStrength(const Item& item) {
    (void)item;
    return false;
}
}  // namespace

bool canUseItem(Ctx& ctx, const Character& ch, const Item& item) {
    (void)ctx;
    if (ch.race == Race::Elf && requiresGreatStrength(item)) {
        return false;
    }
    return true;
}

// R13: Morloch cannot easily charm some monsters [c87].
// UNKNOWN: which monsters are affected is not in the source. No monster is restricted.
bool canCharm(Ctx& ctx, Race race, i32 monsterId) {
    (void)ctx;
    (void)race;
    (void)monsterId;
    return true;
}

}  // namespace mordor
