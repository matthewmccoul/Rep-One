#pragma once

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/attributes.h"
#include "mordor/guilds1.h"
#include "mordor/state_character.h"
#include "mordor/state_item.h"

namespace mordor {

// ---- Constants, enums and small result types (SRD part c07p3) ----

// R2 base attack/defence comes from API kBaseAD (attributes.h); not redeclared here.

// R8 Defend order: no defence bonus is applied.
// R9 intended Defend multiplier. Written in the original but never executed (dead code).
inline constexpr f64 kDefendIntendedBonus = 0.5;

// R13 counting-item resistance replaces the race base with this value (does not stack).
inline constexpr i32 kCountingResistPercent = 90;

// R15 resistance-raising spell sets the recipient's matching resistance to this value.
inline constexpr i32 kResistRaisePercent = 95;

// R18 monster chance-to-hit formula constants (archive article, lowest authority tier).
inline constexpr f64 kMonHitEqualChance = 50.0;
inline constexpr f64 kMonHitSpread = 50.0;
inline constexpr f64 kMonHitFloor = 2.0;
inline constexpr f64 kMonHitCeiling = 98.0;

// R10/R14 damage kinds. Order matches the 11 item resistance bits in R14 and kResistKinds.
enum class DamageKind : i32 {
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

// R14 item resistance bitmask, field 0x30, indexed by DamageKind.
// UNKNOWN: engine's own bit-to-kind mapping; this list comes from the wiki [c88].
inline constexpr u32 kItemResistBits[kResistKinds] = {
    0x001, 0x002, 0x004, 0x008, 0x010, 0x020,
    0x040, 0x080, 0x100, 0x200, 0x400
};

// R16 resistance-raising spell row. A guild level of 0 means "not given" in the source.
// UNKNOWN: whether a blank "Sor" entry means the base level applies (source note says
// no parentheses means base level applies).
// UNKNOWN: guild abbreviation expansions (See, Sor, Wiz, Hea).
// UNKNOWN: Resist Electrical stat line is garbled in the source.
struct ResistSpellRow {
    const char* name;
    DamageKind kind;
    i32 baseLevel;
    i32 seerLevel;
    i32 sorcererLevel;
    i32 wizardLevel;
    i32 healerLevel;
};

inline constexpr i32 kResistSpellCount = 11;

inline constexpr ResistSpellRow kResistSpells[kResistSpellCount] = {
    {"Resist Fire",           DamageKind::Fire,        3,  9, 0,  9, 6},
    {"Resist Draining",       DamageKind::Drain,       3,  9, 0,  9, 6},
    {"Resist Cold",           DamageKind::Cold,        3,  9, 0,  9, 6},
    {"Resist Poison",         DamageKind::Poison,      4, 12, 0, 12, 8},
    {"Resist Disease",        DamageKind::Disease,     4, 12, 0, 12, 8},
    {"Resist Paralysis",      DamageKind::Paralysis,   4, 12, 0, 12, 8},
    {"Resist Stoning",        DamageKind::Stone,       4, 12, 0, 12, 8},
    {"Resist Magic",          DamageKind::Magic,       5, 15, 0, 15, 10},
    {"Mental Resistance",     DamageKind::Mind,        6,  0, 0, 18, 0},
    {"Electrical Resistance", DamageKind::Electrical,  6,  0, 0, 18, 0},
    {"Resist Acidity",        DamageKind::Acid,        6,  0, 0, 18, 0}
};

// R17 spell class as named in the source.
enum class SpellClass : i32 {
    Fire = 0,
    Cold,
    Electrical,
    Mind,
    Damage,
    Kill,
    Banish,
    Dispel,
    Charm,
    Element,
    Protection
};

// R17 resistance type a spell class maps to.
// Element: varies by spell (UNKNOWN per spell). Protection: caster skill only, no target resistance.
enum class ClassResistType : i32 {
    FireResist = 0,
    ColdResist,
    ElectricalResist,
    MindResist,
    MagicResist,
    MagicAndCharmResist,
    ByElement,
    NoTargetResist
};

// R19 argument pair passed to the T06 hit formula on a player-attack call.
// UNKNOWN: T06 hit formula body (only cross-referenced in source [c97]).
struct HitCallArgs {
    i32 monsterDefense = 0;
    i32 characterDefense = 0;
};

// ---- Rule routines ----

// R1 stat part for the character: natural stats plus gear stat bonus (engine StatsADOf).
i32 statsADOf(const Character& c);

// R2 attack total, floored at zero: max(0, kBaseAD + statsADOf + bestGuildAD.att + sum equipped Att).
// R4 CONFLICT: engine statsADOf is used here (preferred); panel term is in R3.
i32 attackOf(const Ctx& ctx, const Character& c, const Inventory& inv);

// R2 defence total, floored at zero: max(0, kBaseAD + statsADOf + bestGuildAD.def + sum equipped Def).
i32 defenseOf(const Ctx& ctx, const Character& c, const Inventory& inv);


// R3 panel variant: kept shift that holds the stat part (StoredADShift).
i32 storedADShift(const Character& c);




// R6 true while the stat part is held (the stored shift only persists while held).
// UNKNOWN: exact criterion for "stat part is held".
bool statPartHeld(const Character& c);




// R9 intended Defend +50% is dead code in the original: returns value unchanged.
i32 applyDefendBonus(i32 value, bool defendOn);

// R10 step 1: race base resistance for a kind (0 if missing).
i32 raceResistBase(const Ctx& ctx, Race race, DamageKind kind);

// R10 step 2 test: any counting equipped item has the kind's resistance bit set.
bool countingItemResists(const Ctx& ctx, const Character& c, const Inventory& inv, DamageKind kind);

// R10 full resistance percent for a kind: race base, then 90 if counting item resists,
// then raised to the spell-resist value if higher.
// UNKNOWN: SpellResist[kind] maps to Resistances.resist[kind].
i32 resistancePercentOf(const Ctx& ctx, const Character& c, const Inventory& inv, DamageKind kind);

// R11 a "counting" item is one whose alignment counts for the wearer.
// UNKNOWN: exact test beyond "of your own alignment".
bool countsForWearer(const Character& c, const Item& item);

// R12 audit form of R10 gives the same result as resistancePercentOf; no separate routine.

// R13 gear does not stack: counting item replaces the race base with 90 (replacement, not max).
// UNKNOWN: whether any race base exceeds 90.
i32 gearResistReplace(i32 raceBase, bool itemResists);

// R14 item resistance bit test for one kind.
// UNKNOWN: 12th resistance flag (race records store 12, item bitmask has 11).
bool itemResistsKind(u32 resistanceFlags, DamageKind kind);



// R18 monster percent chance to hit, clamped to [2, 98].
// CONFLICT: archive article is the lowest authority tier [c99].
f64 monsterHitChance(i32 monAtk, i32 charDef);

// R18 monster hit roll using the chance from monsterHitChance.
// CONFLICT: fractional roll acceptance; monster form Int(Rnd*100)+1 <= chance is used.
// UNKNOWN: whether the monster form is intended.
bool monsterHitRoll(Ctx& ctx, i32 monAtk, i32 charDef);


// R20 monster resistance fields are data only (ResFire ... ResSpecial); no routine declared.
// UNKNOWN: field semantics, value form, and boundaries of ResMagic, ResSpell, ResSpecial.

// R21 resistance display (" -" for 0) is UI-only and out of scope; no routine declared.

}  // namespace mordor
