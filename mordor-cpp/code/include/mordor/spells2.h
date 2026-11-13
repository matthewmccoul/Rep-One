#pragma once
// spells2.h: SRD c10p2 Part B, spell magnitude, range, targeting and resolution.
// R22 (resolution order) is UNKNOWN in the source and has no routine here.
// R23 (cast gate) belongs to T09 and has no routine here.
// R24 (port parity) has no source in this topic and has no routine here.
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"
#include "mordor/casting1.h" // FIXED: SpellUse is declared in casting1.h

namespace mordor {

// ---- constants
inline constexpr double kLnDamageBase = 5.0;          // R2 DmgMod log base LN(5)
inline constexpr double kLnHealBase = 10.0;           // R2 HealMod log base LN(10)
inline constexpr double kArchiveDamageDivisor = 1.61; // R10 archive divisor (sheet uses LN(5))
inline constexpr double kArchiveHealDivisor = 2.3;    // R13 archive divisor "2,3" (sheet uses LN(10))
inline constexpr double kSheetFactor = 1.9;           // R6/R7 sheet multiplier
inline constexpr double kSheetEpsilon = 0.0001;       // R6 floating-point guard for exact integers
inline constexpr i32 kResistGrantPercent = 95;        // R21 resist spell raises matching resistance to 95

// ---- enums
// R1 Type (Spell Data index 4); "N/A" maps to BLANK
enum class SpellEffectType : i32 { BLANK = 0, DETRIMENTAL, BENEFICIAL };
// R8 which targets a cast can reach
enum class SpellReach : i32 { COMBAT_MONSTERS = 0, WHOLE_PARTY, CASTER_ONLY, CHARM_FILTER };
// R9 combat outcome word; UNKNOWN exact verb-table index mapping
enum class SpellVerb : i32 { NONE = 0, KILLED, FRIED, FROZEN, ELECTROCUTED, CHOKED, DISPELLED, CHARMED, SLEPT };
// R12/R21 resistance classes; UNKNOWN mapping to the 11 resist slots
enum class SpellResistKind : i32 { FIRE = 0, COLD, ELECTRICAL, MIND, MAGIC, CHARM };
// R20 displacement or portal result
enum class DisplaceOutcome : i32 { MOVED = 0, ROCK_DEATH, OUTSIDE_DUNGEON };

// ---- small result structs
// R1 one Spell Data row; index -1 when the name is not found
struct SpellRow {
    i32 index;
    SpellEffectType type;
    bool baseBlank;
    i64 base;
    i64 power;
    bool hpFlag;
};
// R3 scaled magnitudes I (damage) and J (heal); blank when Base is blank
struct MagnitudeResult {
    bool blank;
    double dmgMod;
    double healMod;
};
// R4 min and max range; present is false when the range is blank
struct ValueRange {
    bool present;
    i64 min;
    i64 max;
};
// R8 monster cap and group cap per cast
struct SpellTargetCaps {
    i32 numM;
    i32 numG;
};
// Table E per-spell sheet constants (spreadsheet C, Q, R)
struct SheetSpellConst {
    const char* spell;
    double c;
    double q;
    double r;
};
// R16 charm cast shape
struct CharmCastPlan {
    i32 targets;
    i32 groups;
    SpellVerb verb;
};
// R19 stored Sanctuary point; 1-based square and level, zero means never cast
struct SanctuaryStore {
    i32 x;
    i32 y;
    i32 dungeonLevel;
};

// ---- constant tables
// FIXED: kCharmTargets and kCharmGroups are not declared in any indexed header; values set to 0 and must be confirmed against R16
inline constexpr CharmCastPlan kCharmCastPlan{0, 0, SpellVerb::CHARMED}; // R16

// Table E: per-spell constants [c73]
// FIXED: kSheetSpellConstCount is not declared in any indexed header; bound set to the 11 entries listed below
inline constexpr SheetSpellConst kSheetSpellConsts[11] = {
    {"Dominate Devil", 8.0, 1.0, 62.0},
    {"Dominate Humanoid", 8.0, 1.0, 62.0},
    {"Dominate Undead", 9.0, 1.0, 76.0},
    {"Dominate Giant", 10.0, 1.0, 85.0},
    {"Cure Disease", 10.0, 2.0, 23.0},
    {"Piercing Darkness", 10.0, 1.0, 64.0},
    {"Precognative Death", 12.0, 1.0, 110.0},
    {"Soul Entrapment", 12.0, 1.0, 95.0},
    {"Domination", 12.0, 1.0, 115.0},
    {"Retrieve Soul", 14.0, 1.0, 135.0},
    {"Field of Death", 14.0, 1.0, 120.0},
};













// R14 COMBAT or NON_COMBAT; Heal family is NON_COMBAT except Cure Paralysis (COMBAT)
SpellUse spellUse(const Ctx& ctx, i32 spellIndex);








}  // namespace mordor
