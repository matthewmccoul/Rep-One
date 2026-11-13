#pragma once
// companions1b.h: SRD part c12p1, companion rules R27 to R52 (spell or blow, drift and leaving,
// turning, death and carry, Kick Out and right-click Attack, morgue raise, load normalisation,
// slots and names, monster hostility, bind and charm).
// Declarations only. Bodies live in src/companions1b_*.cpp.
// Text screens (R3-style UI prompts and right-click menus) are out of scope; only the rule
// decisions are declared here. Core never prints: all output goes through ctx.say.

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/attributes.h"
#include "mordor/status2.h"
#include "mordor/guilds1.h"
#include "mordor/races.h"

namespace mordor {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

// R27 spell or blow
inline constexpr i32 kCompanionCastLevelDivisor = 6;   // R27.6 companion casts at guild level / 6
inline constexpr f64 kSpellMishapFactor = 1.5;         // R27.9 spell odds x 1.5
inline constexpr f64 kSpellMishapFloor = 1.0;          // R27.9 clamp floor (1)
inline constexpr f64 kSpellMishapCeiling = 40.0;       // R27.9 clamp ceiling (40)
inline constexpr i32 kCasterFizzlePercent = 50;        // R27.5 non-Mage caster gives up half the time
inline constexpr i32 kCasterFailPercent = 50;          // R27.5 companion caster failure 50%

// R28 blow
inline constexpr i32 kBlowFlagCrit = 10;               // R28 Bit(flags,10): crit flag
inline constexpr i32 kBlowFlagBackstab = 11;           // R28 Bit(flags,11): backstab flag
inline constexpr i32 kBlowFlagElectrocute = 6;         // R31 bit 6: electrocute
inline constexpr i32 kBlowFlagStone = 8;               // R30 bit 8: stone
inline constexpr i32 kBlowCritBonus = 4;               // R28 damMod += 4 on crit
inline constexpr i32 kBlowBackstabBonus = 1;           // R28 damMod += 1 on backstab
// CONFLICT: R28 prose says a monster's crit against a character is +2 (c25); the formula gives +4.
// Preferred: the formula (+4) is used for blows.

// R29 / R30 / R31 resistance and stone
inline constexpr f64 kStoneLogBase = 2.5;              // R30 chance uses Log(Max(1,con)) / Log(2.5)
inline constexpr i32 kResistHalfDivisor = 2;           // R29 resist removes half its percentage

// R34 drift and round
inline constexpr i32 kDriftRollRange = 200;            // R34 rng()*200 < Drift
inline constexpr f64 kDriftContentFloor = 0.5;         // R34 content drift minimum
inline constexpr f64 kDriftContentScale = 15.0;        // R34 content drift scale
inline constexpr f64 kDriftBoundFloor = 1.0;           // R34 bound drift minimum
inline constexpr i32 kLeaveRollRange = 200;            // R34 leave test rng()*200 < ChanceToLeave

// R35 chance to leave
inline constexpr f64 kBoundScale = 4.5;                // R35 round_half_even(|binding| * 4.5)
inline constexpr f64 kMonsterLeaveScale = 6.0;         // R35 mon = ln(monsterGL + 1) * 6
inline constexpr f64 kMonsterLeaveWeight = 4.0;        // R35 x = mon * 4
inline constexpr f64 kBoundWeight = 2.0;               // R35 bound * 2
inline constexpr f64 kTopGuildLnDivisor = 10.0;        // R35 ln(topGuildLevel / 10 + 1)
inline constexpr f64 kTopGuildLnWeight = 4.0;          // R35 ln(...) * 4
inline constexpr f64 kChaDivisor = 2.0;                // R35 x = x - charisma / 2
inline constexpr f64 kLeaveNormDivisor = 200.0;        // R35 x = x / 200 * 100
inline constexpr f64 kLeaveNormScale = 100.0;          // R35 x = x / 200 * 100
inline constexpr f64 kHitsLeaveScale = 15.0;           // R35 x += (maxHits - hits) / maxHits * 15
inline constexpr f64 kLeaveFinalDivisor = 2.0;         // R35 x = x / 2
// Describe thresholds (R35 table): 0 is "Completely"; then <=3, <=10, <=15, <=20, else.
inline constexpr f64 kLeaveDescribeLimit[4] = {3.0, 10.0, 15.0, 20.0};

// R36 turning
inline constexpr i32 kTurnEvilPercent = 75;            // R36 Evil creature turns if rnd1*100 < 75
inline constexpr i32 kTurnNeutralPercent = 15;         // R36 Neutral creature turns if rnd2*100 < 15
// CONFLICT: docs c27 give Evil 3 in 4 and Neutral about 1 in 7; engine c8 uses 75 and 15.
// Preferred: engine values above.

// R42 morgue raise
inline constexpr i32 kMorgueRaiseBase = 85;            // R42 raise chance = min(98, 85 + CON)
inline constexpr i32 kMorgueRaiseCap = 98;
inline constexpr i32 kCompanionConIndex = 3;           // R42 Stats[3] is CON

// R47 monster hostility
inline constexpr i32 kNeutralMonsterHostilePercent = 60; // R47 Neutral monsters hostile 60% of the time

// R49 bind spells (index by BindSpell)
inline constexpr i32 kControlBaseLevel = 3;            // R49 Control base 3
inline constexpr i32 kBindBaseLevel = 6;               // R49 Bind base 6
inline constexpr i32 kDominationBaseLevel = 12;        // R49 Domination base 12
inline constexpr i32 kBindReqInt[3] = {12, 14, 18};    // R49 Control, Bind, Domination
inline constexpr i32 kBindReqWis[3] = {12, 14, 18};
inline constexpr i32 kBindReqCon[3] = {0, 0, 18};
inline constexpr i32 kBindReqCha[3] = {10, 16, 18};

// ---------------------------------------------------------------------------
// Enums and small result structs
// ---------------------------------------------------------------------------

enum class CompanionAction : i32 {
    Cast = 0,
    Blow = 1
};

enum class HealRoute : i32 {
    Party = 0,              // heal reaches the party
    MonstersAsDamage = 1,   // heal passed a slot, thrown at monsters as damage
    Nobody = 2              // M5B1D is 0: reaches nobody
};

enum class MishapCaption : i32 {
    HitFor = 0,             // "You were accidently hit for"
    ElectrocutedFor = 1     // "You were accidently electrocuted for"
};

enum class KickOutRefusal : i32 {
    None = 0,
    Dead = 1,               // "drop it first"
    City = 2                // party is in the city
};

struct KickOutCheck {
    bool allowed = false;
    KickOutRefusal refusal = KickOutRefusal::None;
    bool warnBound = false; // binding > 0 earns a warning
};

enum class RightClickOutcome : i32 {
    Ignored = 0,            // Hits <= 0: silent
    Refused = 1,            // both Good
    LetGo = 2               // call letCompanionGo forced
};

enum class TurnOutcome : i32 {
    Joins = 0,              // companion turns and joins the fight
    Holds = 1,              // does not turn
    RefusedByFoe = 2,       // "Your companion refuses to join the current monsters!"
    NoGroupSlot = 3         // no free group slot (of 4)
};

enum class MorgueRaiseOutcome : i32 {
    NoDeadSlot = 0,         // no slot with Hits == 0
    Raised = 1,             // Hits = MaxHits
    Failed = 2              // slot released and left empty
};

enum class DragSwapOutcome : i32 {
    Swapped = 0,
    RefusedDead = 1         // "No use re-arranging a dead companion!"
};

enum class BindSpell : i32 {
    Control = 0,            // increases Bind level
    Bind = 1,               // renews a fading Bind
    Domination = 2          // COMBAT charm; renews Bind level
};

// Live monster set for R41: combat monsters if a fight exists, else the waiting group.
// Both are borrowed; the routine may append a joining companion's body to the live set.
struct LiveSetRef {
    Vec<Creature>& combatMonsters;
    Vec<Creature>& waitingGroup;
    bool fightExists;
    bool groupSlotFree;     // R36.3 a free group slot exists (of 4)
};

// Blow inputs for R28 to R31, taken from the attacking companion's monster body (def).
struct BlowParams {
    u32 specialFlags = 0;   // def flags (crit bit 10, backstab bit 11, stone bit 8, electrocute bit 6)
    i32 damageMod = 0;      // def.DamageMod
    i32 guildLevel = 0;     // def.GuildLevel
    i32 levelFound = 0;     // LevelFound (whose is Unknown, see R30)
    i32 att = 0;            // def.Att
    i32 stat0 = 0;          // Stats[0] (meaning Unknown, see R28)
    i32 stonerCon = 0;      // stoner's CON (R30)
};


// ---------------------------------------------------------------------------
// R28 Blow
// ---------------------------------------------------------------------------
// helper: Bit(flags, n) as used throughout R28 to R31.
bool specialFlagBit(u32 flags, i32 bit);














// R43.4 the carry choice (R37/R38) runs when a member joins or loads (docs) via runCarryChoice.
// R43.1 Ref resolution (R3) is outside this part.

// R45 a released slot keeps its last occupant's record. Name blanking is screen behaviour (out of scope).







// ---------------------------------------------------------------------------
// R52 Bind terms
// ---------------------------------------------------------------------------
// "Bind level" in spell wording is BindingLevel (Companion::bindingLevel). No separate routine.

}  // namespace mordor
