#pragma once
#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/attributes.h"
#include "mordor/progression2.h"

namespace mordor {

// ---- constants ----
inline constexpr i32 kHitCap = 999;                 // R8/R9 maximum-hit ceiling
inline constexpr double kPartyStepPerMember = 0.04; // R2/R4 party size factor step
inline constexpr i32 kChestGuildScale = 333;        // R4 guild level divisor in chest award

// ---- result types ----

// R2 per-blow award and who is credited
struct BlowAward {
    i64 xp = 0;               // XP credited for this blow (0 when no hits inflicted)
    bool creditOwner = false; // true when a companion made the hit (XP goes to owner)
    i32 ownerIndex = -1;      // owner party index when creditOwner, else -1
};

// R5 toolbar state
enum class ToolbarKind : i32 {
    Short = 0,  // XP still below next-level threshold; amount = XP short
    Ready = 1,  // at/above threshold, below cap; amount = XP left to cap
    Pinned = 2  // at or above cap; amount = 0
};

struct ToolbarNeed {
    ToolbarKind kind = ToolbarKind::Short;
    i64 amount = 0;
};

// R10 attack/defence grant rates for a guild
struct LevelAdRates {
    i32 adEvery = 0;     // guild interval (AdEvery)
    i32 adMaxLevel = 0;  // no A/D above this level (AdMaxLevel)
    i32 attPerLevel = 0; // attack increment per award (AttPerLevel)
    i32 defPerLevel = 0; // defence increment per award (DefPerLevel)
};

// ---- R1 damage credit ----

// R1 hits actually removed: min(rawDamage, max(0, targetHits)).
i32 inflictedHits(i32 rawDamage, i32 targetHits);

// ---- R2 per-blow award ----

// R2 per-blow award. Zero when inflicted <= 0; otherwise calls expAwarded (progression2),
// which applies the 0.25 companion factor. Owner routing is returned, not applied.
// CONFLICT: per-blow vs per-kill payout timing; engine per-blow is used. Port banking is UNKNOWN.
BlowAward blowExperience(i64 inflicted, i32 monsterGuildLevel, i32 attackerGuildLevel,
                         bool byCompanion, i32 partySize, i32 companionOwnerIndex);

// ---- R3 clamp at the cap, R6 pinned behaviour ----

// R3 grants `award` XP to guild g of ch, clamped at experienceCap.
// Returns the counted Amount (never below zero; zero when already pinned).
// Pinned line shown only on transition into pinned. Pull-back of total above the pin is not a loss.
// UNKNOWN: full expression for `gained` in payout path [c37].
i64 giveExperience(Ctx& ctx, Character& ch, Guild g, i64 award);

// ---- R4 chest trap disarmed ----

// R4 chest XP. Returns 0 when fightChestTrap <= 0. Level discount uses firstMemberGuildLevel
// (party member 0, not the opener). Party factor applied after first rounding.
// UNKNOWN: disarmChance scale (CONFLICT: 1000 = 100% vs 100%); monsterGL/2 treated as real division.
i64 chestTrapExperience(i32 fightChestTrap, double disarmChance, i32 monsterGuildLevel,
                        i32 firstMemberGuildLevel, i32 partySize);

// ---- R5 pin cap and toolbar ----

// R5 ExperienceCap = Fn13ED(level) = Fn12A1(level+1) - 1 for guild g of ch.
// Uses guild and race exp factors from ctx.data.
i64 experienceCap(const Ctx& ctx, const Character& ch, Guild g);

// R5 toolbar (Progress.ToolbarNeed).
// CONFLICT: engine toolbar uses exp == cap; doc says drained pin shows Pinned. Uses exp >= cap.
ToolbarNeed progressToolbar(const Ctx& ctx, const Character& ch, Guild g);

// ---- R7 level-up gate ----
// R7 gate is canLevelUp (progression2). Engine has no quest term.
// UNKNOWN: whether a quest gate exists in the engine.

// ---- R8 level-up procedure ----
// R8 procedure is levelUp (progression2). This header declares its sub-steps below.

// R8 hit gain for reaching newLevel (before the kHitCap clamp).
// highestOtherLevel: highest level among other guilds (0 if none).
// Gain is zero unless newLevel > highestOtherLevel (strict). Uses rng for the random parts.
// UNKNOWN: rng(n) range and integer vs real; numeric values of AvgHits, LateHits, HitsCapLevel.
i32 levelHitGain(Ctx& ctx, const Character& ch, Guild g, i32 newLevel,
                 i32 highestOtherLevel, const GuildHitsRef& rates);

// R8/R9 clamp max hits to kHitCap.
i32 capMaxHits(i32 maxHits);

// R8 refill current spell points to maximum.
void refillSpellPoints(Character& ch);

// R8 recalculate combat swings after a level.
// UNKNOWN: swings formula is not in the chunk.
void recalculateSwings(Character& ch);

// ---- R9 level-up hits and stats ----
// R9: stats do not rise on a level. No function is needed; see levelHitGain and capMaxHits.

// ---- R10 attack and defence grants ----

// R10 award-level step (fn13D4): ((Int(cap)/2) + (acc/18)) / (adEvery + bonus), rounded to integer.
// UNKNOWN: acc and bonus values.
i32 adAwardStep(i64 experienceCapValue, i32 acc, i32 adEvery, i32 bonus);

// R10 whether newLevel is an A/D award level: level <= adMaxLevel and level % step == 0.
// CONFLICT: text says level mod Int(AdEvery) = 0; engine uses step (port matches 332/332). Step is used.
bool isAdAwardLevel(i32 level, i32 step, i32 adMaxLevel);

// R10 add attack and defence to guild g of ch when newLevel is an award level.
void grantLevelAd(Ctx& ctx, Character& ch, Guild g, i32 newLevel, i32 awardStep,
                  const LevelAdRates& rates);

// ---- R11 level-up gold ----
// R11 cost is goldForNextLevel (progression2). Deduction is UNKNOWN; no function declared here.

// ---- R12 XP threshold ----

// R12 fixed-point rounding to four decimal places (half-even), returned scaled by 10000.
i64 curFixed(double x);

// R12 race multiplier f from r = raceExp + 1:
// ((Ln(r)/Ln(r+1)) + 1) - 0.7 + r/25
double raceExpMultiplier(double raceFactorPlusOne);

// R12 Fn12A1(level): XP needed to reach `level`, from guild exp coefficient and race exp factor.
// Pure. UNKNOWN: return value at level 0 (forfeit path passes 0 at level 1).
i64 levelThresholdXp(i32 level, double guildExp, double raceExp);

}  // namespace mordor
