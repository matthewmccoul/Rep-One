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

// ---- rule routines
// R1 find a spell by exact name in Spell Data B3:J103; index -1 when absent
SpellRow spellRowByName(const Ctx& ctx, const Str& spellName);

// R2 DmgMod I = G + G*LN(SL)/LN(5) + INT((SL/10)*(H-1)); SL is clamped 1..255 by the caller
// UNKNOWN: meaning of SL (`$C$2`); the caller supplies it
double damageMod(i64 base, i64 power, i64 sl);
// R2 HealMod J = G + G*LN(SL)/LN(10) + INT((SL/10)*(H-1))
double healMod(i64 base, i64 power, i64 sl);

// R3 guarded magnitude: blank when the row's Base is blank
MagnitudeResult spellMagnitude(const SpellRow& row, i64 sl);

// R4 Detrimental damage range L/M = ROUNDDOWN(I*0.5-1) and ROUNDDOWN(I*1-1)
// CONFLICT: c74 ternary writes floor(); ROUNDDOWN is toward zero. Following ROUNDDOWN.
ValueRange damageRange(SpellEffectType type, const MagnitudeResult& mag);
// R4 Beneficial heal range N/O = ROUNDDOWN(J*0.5-1) and ROUNDDOWN(J*1-1)
// Quirk: sheet tests N for O and L for M; reproduced by using the type and blank state directly
ValueRange healRange(SpellEffectType type, const MagnitudeResult& mag);

// R5 HP note shown when a damage range exists and the spell's flag is "Y" (text only, not enforced)
bool hpNoteShown(const ValueRange& dmgRange, const SpellRow& row);

// R6 sheet base value D = floor(Q*R*1.9 - 0.0001)
i64 sheetBaseValue(double q, double r);
// R6 sheet scaled base E: X = floor(C*Q*R*1.9/SL - 0.0001); E = C if X < C else X
// UNKNOWN: blank propagation of E (c75) is not modelled here
i64 sheetScaledBase(double c, double q, double r, i64 sl);

// R7 sheet column K = ROUNDUP(C*Q*R*1.9/(C+1), 0)*2 - 1 (ROUNDUP away from zero)
// UNKNOWN: purpose of column K is not stated
i64 sheetColumnK(double c, double q, double r);

// R8 monster cap numM and group cap numG for a spell row
// UNKNOWN: exact max-vs-exact semantics of numM; field names come from the spell data
SpellTargetCaps spellTargetCaps(const Ctx& ctx, i32 spellIndex);
// R8 target reach class for a spell row
SpellReach spellReach(const Ctx& ctx, i32 spellIndex);

// R9 effect verb carried by a combat spell
// UNKNOWN: mapping from the spell's AffectMonster to the verb table
SpellVerb spellVerb(const Ctx& ctx, i32 spellIndex);

// R10 archive damage minimum = (Base + Base*LN(SL)/1.61 + Int(SL*(Power-1)/10)) / 2
double archiveDamageMinimum(i64 base, i64 power, i64 sl);
// R10 archive base damage = Minimum + Random(1 to Minimum)
// UNKNOWN: Minimum is fractional; the roll uses floor(minimum) as the upper bound
i64 archiveDamageRoll(Ctx& ctx, double minimum);
// R10/R12 final damage = Integer(damage * (1 - Resistance/100)), VB Integer rounding (half to even)
// UNKNOWN: resistance combination (floor, cap or stack) is not stated in R12
i64 resistedDamage(i64 damage, i32 resistancePct);

// R11 kill spell lands only when the rolled damage reaches the monster's hits (engine effect 100)
// CONFLICT: archive says damage < HP gives 0; manual says kill outright or no effect. Engine order chosen.
bool killSpellSucceeds(i64 rolledDamage, i64 monsterHits);

// R13 archive heal minimum = (Base + Base*LN(SL)/2.3 + Int(SL*(Power-1)/10)) / 2
double archiveHealMinimum(i64 base, i64 power, i64 sl);
// R13 archive healing = Minimum + Random(1 to Minimum)
// UNKNOWN: Minimum is fractional; the roll uses floor(minimum) as the upper bound
i64 archiveHealRoll(Ctx& ctx, double minimum);

// R14 COMBAT or NON_COMBAT; Heal family is NON_COMBAT except Cure Paralysis (COMBAT)
SpellUse spellUse(const Ctx& ctx, i32 spellIndex);

// R15 item protection added defence = 3 * LN(SL)^2 (current guild spell level)
// UNKNOWN: (LN SL)^2 is taken as written; LN(SL^2) is not chosen
double itemProtectionDefence(i64 sl);
// R15 guild spell protection added defence = 3 * LN(Int(SL/2)+1)^2 (highest joined SL)
// UNKNOWN: forum post cited as the source of the bug note is not provided
double guildSpellProtectionDefence(i64 sl);

// R16 charm converts monsters of the filtered class into companions; returns number charmed
// UNKNOWN: charm path under the class gate (c27 vs c65); companion creation is delegated
i32 charmMonsters(Ctx& ctx, Party& party, Encounter& enc, i32 monsterClassFilter);

// R17 bind raises the companion's bind level; returns false and changes nothing if joined freely
bool bindCompanion(Ctx& ctx, Companion& companion, i32 bindLevelGain);

// R18 Control Dragon failure: targets respond adversely (event only)
// UNKNOWN: any further failure mechanics
void controlDragonFailed(Ctx& ctx, Encounter& enc, i32 groupIndex);

// R19 Set Sanctuary stores the caster's location
// UNKNOWN: Location coordinate base; stored values are 1-based per the SRD terms
void setSanctuary(Ctx& ctx, SanctuaryStore& store, const Location& at);
// R19 Sanctuary returns the caster and party to the stored point and ages the caster by kSanctuaryAgeDays.
// Does nothing and returns false if never set.
// CONFLICT: manual says caster and party return; archive says caster only. Manual followed.
bool castSanctuary(Ctx& ctx, Party& party, i32 casterIndex, const SanctuaryStore& store);

// R20 displacement moves the party within the level; right and forwards are relative to facing
// Outside the dungeon or into rock kills the caster and party.
DisplaceOutcome displaceWithin(Ctx& ctx, Party& party, i32 casterIndex, i32 offsetRight, i32 offsetForward);
// R20 Ethereal Portal moves through the wall in front; a rock cell behind kills the whole party
DisplaceOutcome etherealPortal(Ctx& ctx, Party& party, i32 casterIndex);

// R21 resist spell sets the matching resistance to kResistGrantPercent (NON_COMBAT)
// UNKNOWN: whether this is a floor, cap or stack is not stated
void castResistSpell(Ctx& ctx, Resistances& resistances, SpellResistKind kind);

}  // namespace mordor
