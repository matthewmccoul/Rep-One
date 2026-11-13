#pragma once

// progression2: SRD part c05p2 (experience, levels, raising, aging).
//
// Conventions for this header:
//  - Rules that draw randomness or emit messages take Ctx& first.
//  - Pure queries over state and the constant tables below take no Ctx.
//  - Rule numbers are cited where the part numbers them. Other routines carry
//    their source chunk tag ([cNN]) instead, because this part does not number them.
//  - CInt(x) is roundHalfEven() from math_util.h and is not redeclared here.
//  - rng(n) is ctx.rng.below(n) and rng() is ctx.rng.nextDouble(). Range is UNKNOWN in the source.
//  - Currency values are i64 in 1/10000 gold.

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"

namespace mordor {

struct GameState;  // defined in state_game.h; implementation includes it
struct GuildHitsRef;  // FIXED: defined in attributes.h; forward-declared here because only a const reference is used

// ---------------------------------------------------------------------------
// Constants (SRD tables T1, T2, T10 and the formula constants of c104/c61/c18)
// ---------------------------------------------------------------------------

// T1 RaceMult by Race enum order (Human..Troll). [c61][c71][c96][c104]
inline constexpr double kRaceExpMult[9] = {
    1.00000000000,  // Human
    1.18544741085,  // Elf
    1.18544741085,  // Giant
    1.18544741085,  // Gnome
    1.10551994263,  // Dwarf
    1.14714602440,  // Ogre
    1.05819132268,  // Morloch
    1.22165868435,  // Osiri
    1.25647909518   // Troll
};

// T2 guild EP (XP factor; Parameter = EP x 8) by Guild enum order. [c71][c88]
// Healer's Parameter is not printed in the source; EP 4.0 is taken from [c71].
// UNKNOWN: Wizard Parameter 27 [c88] vs EP 3.75 (Parameter 30) [c71]; EP 3.75 used.
inline constexpr double kGuildExpFactor[kGuildCount] = {
    1.0,    // Nomad
    2.0,    // Warrior
    2.375,  // Paladin
    2.625,  // Ninja
    2.625,  // Villain
    2.25,   // Seeker
    1.875,  // Thief
    2.25,   // Scavenger
    3.375,  // Mage
    3.375,  // Sorcerer
    3.75,   // Wizard
    4.0     // Healer
};


// Level XP closed form (engine-aligned, test convention). [c61][c104]
// xp = kXpQuadCoef * l*l * EP + kXpLinCoef * (2*l - 1) - kXpConst, l = level - 1
// total = roundHalfEven(xp * RaceMult)
inline constexpr double kXpQuadCoef = 88.09020776;
inline constexpr double kXpLinCoef = 45.45454;
inline constexpr double kXpConst = 0.264;
// UNKNOWN: archive level formula (constants 266, 177 [c103]) gives 889,450 at L100; not used.

// Gold closed form (engine L^2 cumulative form). [c24][c27][c104]
inline constexpr double kGoldCoef = 30.1;
inline constexpr double kGoldFactorDivisor = 10.0;  // (1 + GGF / 10)


// Party penalty per extra member. [c18][c104]
inline constexpr double kPartyPenaltyPerMember = 0.04;

// Kill XP constants. [c18][c104]
inline constexpr double kKillLnHitsA = 9.0;     // (9 - Ln(h + 1))
inline constexpr double kKillLnHitsB = 20.0;    // (20 - Ln(h))
inline constexpr double kKillMonsterOffset = 50.0;  // (monsterGL + 50)
inline constexpr double kKillDivisor = 1000.0;
inline constexpr double kCompanionShare = 0.25;  // companion strike factor [c18][c104]

// Chest XP constant: xp = disarm * (monsterGL * 0.5) / (1 + firstGL / 333). [c104]
inline constexpr double kChestGuildShare = 0.5;


// GuildLevelFactor: 1 / (1 + guildLevel / 333). [c104][c18]
double guildLevelFactor(i32 guildLevel);

// ---------------------------------------------------------------------------
// Experience queries for a character's guild (pure)
// ---------------------------------------------------------------------------

// ExperienceCap for the character's guild, using its EP and race. [c3][c36]
i64 experienceCapOf(const Character& ch, Guild g);

// ExpForThisLevel: 0 at level 1, else Fn12A1(level - 1). [c36]
i64 expForThisLevel(const Character& ch, Guild g);

// ExpForNextLevel: Fn12A1(level). [c36]
i64 expForNextLevel(const Character& ch, Guild g);

// IsPinned: Experience >= ExperienceCap. The quest term is absent from the engine. [c3][c37]
bool isPinned(const Character& ch, Guild g);

// CanLevelUp: level < kMaxGuildLevel and Experience >= Fn12A1(level). [c2][c3]
// UNKNOWN: engine MaxLevel and quest gate [c2][c91][c102].
bool canLevelUp(const Character& ch, Guild g);

// GoldForNextLevel: Fn12B2(level, GGF). [c2][c24]
i64 goldForNextLevel(const Character& ch, Guild g);

// ToolbarState: Short if Experience < ExpForNextLevel; Pinned if Experience == cap; else Ready. [c17]
// CONFLICT: toolbar uses ==, IsPinned uses >=. Toolbar test is kept as written. [c3][c17]
// FIXED: ToolbarState is not declared anywhere in the API index; using the closest existing type i32 (state code: Short/Pinned/Ready).
i32 toolbarState(const Character& ch, Guild g);

// TotalExperience: sum of Experience over all guilds. [c1][c11]
i64 totalExperience(const Character& ch);

// HighestOtherLevel: max level of guilds other than g; 0 if none. [c2]
i32 highestOtherLevel(const Character& ch, Guild g);

// ---------------------------------------------------------------------------
// Kill and chest XP (pure arithmetic)
// ---------------------------------------------------------------------------

// Kill XP. Round(PartyFactor * Round(xp)), zero when inflicted h == 0. [c5][c18][c23]
// xp = h*(9-Ln(h+1))*(20-Ln(h))*(monGL+50)*(byCompanion ? 0.25 : 1)*(1/(1+guildLevel/333))/1000
// CONFLICT: wiki floors and omits final round [c87]. Engine used. [c23]
i64 expAwarded(i64 inflicted, i32 monsterGuildLevel, i32 guildLevel, bool byCompanion, i32 partySize);

// Chest XP (R4). Round(party * Round(xp)), xp = disarmChance * (monsterGL / 2) / (1 + firstGL / 333). [c18][c27]
// UNKNOWN: scale of disarmChance (1000 = 100% or 0..1) [c18][c27][c61]; disarmChance is
// passed as a fraction in 0..1 here. Party factor is applied per CONFLICT [c104] resolved to engine.
i64 chestAward(i32 partySize, double disarmChance, i32 monsterGuildLevel, i32 firstCharGuildLevel);

// AwardExp: adds amount clamped to [0, experienceCap - total]. Zero when pinned. Experience is
// clamped at ExperienceCap. Returns the granted amount. [c5][c8][c37]
// CONFLICT: wiki says "XP keeps accumulating" while pinned [c91]; engine (no gain) used.
i64 awardExp(Character& ch, Guild g, i64 amount);

// ---------------------------------------------------------------------------
// Level-up and hits (R18-style, source chunk [c2])
// ---------------------------------------------------------------------------

// LevelUp: Level++, then add hits by branch, clamp MaxHits to kMaxHits, set CurHits = MaxHits.
// Branch: aheadOfEvery = newLevel > highestOther.
//   if newLevel > hitsCapLevel or not aheadOfEvery: add lateHits only when aheadOfEvery.
//   else: if rng(3)+1 <= 2: MaxHits += int(rng * avgHits / 5) + 1; MaxHits += avgHits;
//         if CON > 16 and newLevel < hitsCapLevel: MaxHits += int(rng * (CON-16)) + 1.
// UNKNOWN: rng(n) range; mapped to ctx.rng.below(n) and ctx.rng.nextDouble() [c2].
// UNKNOWN: whether gold is deducted on level-up [c2].
// FIXED: GuildHitRates is not declared anywhere in the API index; using the closest existing type GuildHitsRef.
void levelUp(Ctx& ctx, Character& ch, Guild g, const GuildHitsRef& rates);

// ---------------------------------------------------------------------------
// Forfeit (source chunk [c3][c49])
// ---------------------------------------------------------------------------

// ForfeitCost: levels to demote. UNKNOWN: log base [c49].
i32 forfeitCost(const Character& ch, Guild g);

// ForfeitGuild: level = max(1, from - cost); Experience = Fn12A1(level - 1). [c3]
void forfeitGuild(Character& ch, Guild g);

// ---------------------------------------------------------------------------
// Attack/defence award step and interval (source chunk [c10][c28])
// ---------------------------------------------------------------------------

// AdAllowed: level Mod Int(adEvery) == 0 and level <= adMaxLevel. Zero interval never allowed.
// UNKNOWN: AdEvery and AdMaxLevel values per guild [c10][c106].
bool adAllowed(i32 level, double adEvery, i32 adMaxLevel);

// AwardStep: ((Int(cap) / 2) + acc / 18) / (adEvery + bonus), rounded to integer. [c28]
// UNKNOWN: acc and bonus definitions [c28][c35].
i32 awardStep(i64 cap, double acc, double adEvery, double bonus);

// Fn13D4: next level on which an award falls, with integer rounding applied. [c28]
// UNKNOWN: exact inputs; (currentLevel, stepInterval) assumed.
i32 nextAwardLevel(i32 currentLevel, i32 stepInterval);

// ---------------------------------------------------------------------------
// Aging (T3 events, source chunk [c2][c4][c31][c34][c87])
// ---------------------------------------------------------------------------

// AgeYears: Age / 365 (integer). [c2][c53]
i32 ageYears(const Character& ch);

// DescribeDays: weeks and days wording. 7 days prints "7 day(s)" because of "days > 7". [c2]
Str describeDays(i32 days);

// AgeBy: adds days to ch.ageDays and prints the aging message. UNKNOWN: message wiring for
// monsters, chest Withering, raises and Kind 25 items [c34][c4].
void ageBy(Ctx& ctx, Character& ch, i32 days);

// Withering (trap 10): AgeBy(kWitheringDays). [c4][c87]
void withering(Ctx& ctx, Character& ch);

// Kind 25 stat item: AgeBy(kKind25AgeDays). UNKNOWN: item identity [c31].
void useKind25(Ctx& ctx, Character& ch);

// Sanctuary cast: caster ages kSanctuaryAgeDays (original). Target UNKNOWN: caster only is used
// here, per R31 "The caster ages approximately one year" [c31][c34][c106].
void castSanctuary(Ctx& ctx, Character& caster);

// Complication: Age += 15 years; MaxHP *= kComplicationHpFactor. [c87][c103]
// CONFLICT: archive 0.8333 vs research 5/6 [c82][c103]; 0.8333 used.
void complication(Character& ch);

// ComplicationChance: min(1, 2 * ageYears / maxAgeYears). Returns a probability in 0..1. [c82][c61]
double complicationChance(const Character& ch, i32 maxAgeYears);

// ConLossChance: max(0, 50 - 2*CON) percent. [c82][c61]
i32 conLossChance(i32 con);

// RaiseCharacter: raise aging (kRaiseAgeDays), CON-loss roll using conLossChance, and a
// stat -5 with floor at the racial minimum. UNKNOWN: which stat takes the -5. This design
// applies it to CON when the roll succeeds [c61][c87][c103].
// racialMinimums is supplied by the caller from the race table (not in this header).
void raiseCharacter(Ctx& ctx, Character& ch, const Arr<i32, kStatCount>& racialMinimums);

// RestInCity: returns rest days ages the character by them. Days =
// restRoundB((100 - 100*curHP/maxHP) / kRestCostDivisor). Zero HP = kRestZeroHpDays.
// UNKNOWN: healing on arrival [c24][c48]; CONFLICT with wiki 32/16 day table [c87].
i32 restInCity(Ctx& ctx, Character& ch);

// B(x): rounding used by the town rest age cost. UNKNOWN: rounding rule [c23][c24][c25].
i32 restRoundB(double x);

// ---------------------------------------------------------------------------
// Play clock (source chunk [c59])
// ---------------------------------------------------------------------------

// PlayClockTick: if not paused, playElapsedMs += nowMs - lastTickMs; then lastTickMs = nowMs.
// lastTickMs refreshes even when paused. The census names PlaySeconds, but GameState
// stores milliseconds in playElapsedMs [c59]. CONFLICT: ms used.
void playClockTick(GameState& game, i64 nowMs);

}  // namespace mordor
