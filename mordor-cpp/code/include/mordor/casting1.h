#pragma once
// casting1.h: SRD c09p1, magic and casting.
// Covers spell use classification, the cast gate and refusal order, spell level,
// reach, spell cost, spell points, item and buffer casting, monster spells,
// breath, charms, protection, map and utility effects, and seer fees.
// Declarations only. Routines whose source is not engine-verified are marked UNKNOWN or CONFLICT.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"
#include "mordor/state_companion.h"
#include "mordor/state_item.h"
#include "mordor/state_dungeon.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- Constants ----------------------------------------------------------

inline constexpr i32 kSpellSchoolCount = 16;          // R65 schools Fire 0 .. Opening 15
inline constexpr i32 kSpellClassCount = 18;           // R65 spell classes
inline constexpr i32 kSpellLevelMin = 1;              // R21 clamp floor
inline constexpr i32 kSpellLevelMax = 255;            // R21 clamp ceiling
inline constexpr i32 kPeacedSpellLevel = 7;           // R6 TooLongPeaced threshold (spell level > 7)
inline constexpr i32 kMapSize = 30;                   // R55 30x30 grid
inline constexpr i32 kBitNotRock = 23;                // R56 "not rock" square bit
inline constexpr i32 kNoSchool = -1;                  // R49 no school sampled (attempt cap)
inline constexpr i32 kMonsterSchoolAttemptCap = 1001; // R49 sampling cap (what follows is UNKNOWN)
inline constexpr i32 kMonsterCastSkipPercent = 10;    // R49 flat skip check
inline constexpr i32 kMonsterNonMageSkipPercent = 50; // R49 extra skip check for non-mage classes
inline constexpr i32 kBreathBaseChance = 50;          // R50 percent
inline constexpr i32 kBreathHighChance = 80;          // R50 percent when caster top level is high
inline constexpr i32 kBreathLowChance = 20;           // R50 percent when caster top level is low
inline constexpr i32 kCategoryCharm = 7;              // R65 category id Charm
inline constexpr i32 kCategoryHeal = 9;               // R65 category id Heal
inline constexpr i32 kGroupAffectMin = 6;             // R65 AffectGroup >= 6 is group spell
inline constexpr i32 kItemOnlySpellLevel = 50;        // R67 spells at level 50 are item-only
inline constexpr i32 kFateRollSides = 3;              // R73 unused Int(Rnd * 3) roll
inline constexpr i32 kCastRollSides = 100;            // R12 Int(Rnd * 100)
inline constexpr i32 kEffectWholePartyOffset = 200;   // R64 whole-party variant offset
inline constexpr i32 kEffectPromptId = 18;            // R64 "Cast on Who?" prompt effect
inline constexpr i32 kEffectBindingFirst = 40;        // R64 binding effects 40..42
inline constexpr i32 kEffectBindingLast = 42;
inline constexpr i32 kEffectLifeDomination = 150;     // R64 binding, sets ForLife
inline constexpr i32 kEffectResistFirst = 50;         // R64 "Resists" effects 50..60
inline constexpr i32 kEffectResistLast = 60;
inline constexpr i32 kEffectCharmFirst = 75;          // R64 charm effects 75..99
inline constexpr i32 kEffectCharmLast = 99;
inline constexpr i32 kEffectKillTargets = 100;        // R64 "Kills target(s) or has no effect"
inline constexpr f64 kSeerFeeConstant = 132567.52;    // R63 seer fee constant

// ---- Enums and small result structs -------------------------------------

// R1 spell use class (derived, never stored)
enum class SpellUse : i32 { NonCombat = 0, Combat = 1, Either = 2 };

// R4-R9 cast refusal kinds, in the order the gates test them
enum class CastRefusal : i32 {
    None = 0,
    NotKnown,       // R4 unfit gate, step 1
    SpellLevel,     // R4 unfit gate, step 2
    Stat,           // R4 unfit gate, step 3
    SpellPoints,    // R5 row 1
    NonCombat,      // R5 row 2
    AntiMagic,      // R5 row 3
    CombatOnly,     // R5 row 4a
    TooLongPeaced   // R5 row 4b
};

// R13 whether a cast happens now or becomes the standing order
enum class CastDisposition : i32 { CastNow = 0, BecomesOrder };

// R50 breath attack result
enum class BreathOutcome : i32 {
    NoBreath = 0,      // chance roll missed
    Breathed,          // damage applied
    EndTurn,           // monster level 0 ends the whole turn
    EndGroupTurn       // Log(0) raised, resume at End Sub, skip rest of group turn
};

// R61 what a buffer press does
enum class BufferFire : i32 { Nothing = 0, UseItem, StandingOrder, CastNow };

// R59 timed ailments set by spells
enum class SpellAilmentKind : i32 { Levitate = 0, SightVeil, SeeInvisible, Protection };

// R5-R9 facts the cast gate reads from the square and encounter
struct CastSituation {
    bool monstersPresent = false;  // fn0EEF() > 0 (R6); CastGate flag (R8) is supplied by the caller
    bool engaged = false;          // Foes(0).Engaged (R6)
    bool antiMagicSquare = false;  // ANTIMAGIC or EXTINGUISHER at position (R6, R49.6)
};

// R10 / engine item combat round: which item is casting
struct ItemCastRequest {
    i32 memberIndex = 0;   // party member casting the item
    i32 itemSlot = -1;     // pack slot of the item
    i32 itemIndex = -1;    // item index used in ItemOrders
    i32 itemLevel = 0;     // item spell level; 0 means a level-0 item (R18, R20)
    bool fromBuffer = false;
};

// R49 monster spell pool bounds
struct MonsterSpellBand {
    i32 low = 1;
    i32 top = 1;
};

// R50 breath attack result
struct BreathResult {
    BreathOutcome outcome = BreathOutcome::NoBreath;
    i32 damage = 0;
};

// ---- Rule routines ------------------------------------------------------

// R1 spell use class, derived from raw spell fields each time
SpellUse spellUse(const SpellDef& spell);


// R4 unfit gate: Known, SpellLevel, Stat in order; returns None if the caster passes
CastRefusal unfitGateCheck(const Ctx& ctx, const Character& who, const SpellDef& spell);











// R16 pay the cast cost and set the cast flag that item charges depend on
void payCastCost(Character& caster, i32 cost);

// R18 cast cost: 0 when the item level is non-zero, otherwise the engine cost (R28)
i32 castCostSp(const Ctx& ctx, const Character& caster, const SpellDef& spell, i32 itemLevel);




// R21 spell level from guild level: clamp(floor(guildLevel / 2 + 0.5), 1, 255)
i32 guildSpellLevel(i32 guildLevel);


// R23 per-guild reach test: spellLevel * rate <= guildLevel / 2 + 0.5 (exact comparison)
bool guildReachesSpell(i32 spellLevel, f32 rate, i32 guildLevel);

// R23 a spell is within reach when some joined guild (Level > 0) has rate > 0, a ceiling
// reaching the level, and passes guildReachesSpell
bool spellWithinReach(const Ctx& ctx, const Character& caster, const SpellDef& spell);

// R23 ids of all spells within reach for this character
Vec<i32> listReachableSpellIds(const Ctx& ctx, const Character& caster);






// R28 engine cost: cheapest accepted guild sets the cost, clamped up to the smallest accepted rated level.
// Ratio-0 spells give 0 (fn1485).
// CONFLICT: published test page gives -1 for ratio 0 (see spellCostPublished).
i32 spellCostEngine(const Ctx& ctx, const Character& caster, const SpellDef& spell);
















// R39 effectiveness uses the teaching guild's spell level and the lowest cost among guilds.
// Covered by casterSpellLevel (R22) and spellCostEngine (R28); no separate routine.

// R40 archive COMBAT/NON-COMBAT restriction. Not used; the cast gate (R5/R6) applies.
// CONFLICT: archive says "peaceful" is undefined and compares against R6.



// R46 natural max SP: ((INT * 10) / 2) + ((WIS * 10) / 2), natural unmodified stats
i32 naturalMaxSpellPoints(const Abilities& abilities);

// R47 recalculate max SP; refill sets cur = max, otherwise cur = min(cur, max)
void recalcSpellPoints(Character& who, bool refill);



































// R62 an identified square is written to the automap; already-mapped squares say nothing
void perceiveSquare(Ctx& ctx, SquareState& square);






// R64 / R65 binding spell: effects 40..42 or 150 (Life Domination sets ForLife)









// R67 highest spell ceiling for a category over guilds with HasSpellTable
i32 spellCeiling(const Ctx& ctx, i32 category);

// R67 a spell is unreachable when its level exceeds the category ceiling
bool spellUnreachable(const Ctx& ctx, const SpellDef& spell);






// Quirks and deviations R14, R17, R44, R58, R68, R69, R70, R72, R74-R76, R79-R85 are
// covered by the routines above or are UI and data notes; no separate routines are declared.

}  // namespace mordor
