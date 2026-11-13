#pragma once

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"

// progression1b.h: SRD part c05p1, rules R13 to R27.
// Functions already in the API INDEX (ageYears, describeDays, withering,
// complication, complicationChance, restInCity, forfeitCost, Forfeit,
// goldForNextLevel, expForNextLevel, guildLevelFactor) are not redeclared here.
// All gold values in this module are WHOLE gold (SRD units). Convert with
// currencyFromWhole() before storing in Currency (1/10000 gold).

namespace mordor {

// ---- Constants and SRD tables ----

// R13 closed-form constants (test form is the reference)
inline constexpr double kGefDivisor = 8.0;                  // R13 GEF / 8
// R13 spreadsheet form, no rounding, uses kXpLinCoefExact. Returns 0 for lvl <= 1.
double closedFormLevelXpUnrounded(i32 lvl, double guildEp, double raceFactor);
// R13 research form. CONFLICT: uses -1 inside the bracket (see kXpOffset). guildGef is the printed E.
double closedFormLevelXpResearch(i32 lvl, double guildGef, double raceFactor);
// R13 wiki form (GUILD_MOD = GEF/8, RACE_MOD = Actual Exp). Not rounded.
double closedFormLevelXpWiki(i32 lvl, double guildGef, double raceActualExp);
// R13 wiki c88 form: XP(L) = round((i + j - 2) * (k + l + 0.3) / 10), a = raceFactor + 1
i64 levelXpWikiC88(i32 lvl, double guildFactor, double raceFactor);
// R13 XPforPin = XP(min(currentLevel + 1, 998)) using the wiki c88 form
i64 xpForPin(i32 currentLevel, double guildFactor, double raceFactor);
// R13 archive level XP: r * g * (n/2) * (266 + (n-1) * 177). Unreconciled with the engine.
// UNKNOWN: per-guild g values are not given in the chunk.
double levelXpArchive(i32 n, double raceFactor, double guildFactor);
// R13 XP still needed = XP_Total - current XP
i64 xpStillNeeded(i64 xpTotal, i64 currentXp);

// ---- R14 Race factors and guild values ----
// R14 RaceMult for a raw race Exp (T1 test form). Raw Exp is clamped to 0..9.
double raceMultForRawExp(i32 rawExp);
// R14 approximation: 1 + [ln(Exp)/ln(3) - 1] / [4.50 - (Exp-4) * (11/90)]. Community, not authoritative.
double raceActualExpApprox(i32 rawExp);
// R14 archive race restrictions: Elves and Osiri not Warrior; Giants and Morlochs not Wizard; Gnomes neither
bool raceAllowsGuild(Race race, Guild guild);
// R14 wiki stat item cap: race natural max + 5
i32 statItemMaxAllowed(i32 raceNaturalMax);
// R14 wiki percentage from a printed guild Parameter: Parameter / 8 * 100
double guildParameterPercent(i32 parameter);

// ---- R15 Fast projection (approximate, not authoritative) ----
// R15 PROJECTED_XP = (DESIRED_LEVEL / CURRENT_LEVEL)^2 * CURRENT_XP.
// Caller multiplies by the race factor for non-Human races.
double projectedXp(i32 desiredLevel, i32 currentLevel, double currentXp);
// R15 PROJECTED_LEVEL = (DESIRED_XP / CURRENT_XP)^(1/2) * CURRENT_LEVEL.
// UNKNOWN: behaviour when currentXp or currentLevel is 0.
double projectedLevel(double desiredXp, double currentXp, i32 currentLevel);

// ---- R16 Gold threshold (whole gold) ----
// R16 combat-audit: total = sum_{i=1..L} (2i-1)*30.1, rounded each step; cost = round(total*(1+GGF/10))
i64 goldCostAuditRunning(i32 level, i32 ggf);
// R16 archive loop: LevelCost += round((glvl*2-1)*30.1); gold to reach glvl+1 = round(LevelCost*(1+GGF/10)). Rounds x.5 up.
i64 goldToReachArchiveLoop(i32 glvl, i32 ggf);
// R16 archive "per level" closed form: 30.1 * n^2 * (1+ggf/10). CONFLICT: equals the cumulative loop value at glvl=n, not one step.
i64 goldPerLevelArchiveClosed(i32 n, i32 ggf);
// R16 archive total closed form, levels 1 to n+1: n(n+1)(2n+1)/6 * 30.1 * (1+ggf/10). Page: may be off by up to 999 gp.
i64 goldTotalArchiveClosed(i32 n, i32 ggf);
// R16 research form: P * floor(30.1 * x^2 + x/10), P = 1 + ggf/10. UNKNOWN: operator between P and floor.
i64 goldNextLevelResearch(i32 x, i32 ggf);
// R16 wiki c88 form: floor(floor(i + j) * (k + 1)), i = L^2*30.1, j = L/10, k = goldFactor/10
i64 goldNextLevelWikiL2(i32 level, i32 goldFactor);
// R16 wiki c96 form: floor((L-1)^2 * 30.1 + (L-1)/10) * GP, GP = goldFactor/10 + 1. Not authoritative.
i64 goldNextLevelWikiMinus1(i32 level, i32 goldFactor);
// R16 wiki c88 total: sum over n = 1..level of goldNextLevelWikiL2(n, goldFactor)
i64 goldTotalWiki(i32 level, i32 goldFactor);

// ---- R17 Forfeit (pinned character, in guild) ----
// R17 new guild level after forfeit: max(1, from - cost). Forfeit itself is in the API index.
// UNKNOWN: ForfeitCost log base; forfeit to level 1 passes 0 to Fn12A1.
i32 forfeitTargetLevel(i32 fromLevel, i32 cost);
// R17 archive: levels lost remove only AH hits; bonus hits from those levels are kept.
// UNKNOWN: storage field for AH hits; archive tier only.
void forfeitLoseLevelHits(Ctx& ctx, Character& ch, Guild guild, i32 levelsLost);
// R17 archive: regaining levels below the top guild level grants extra bonus hits.
// UNKNOWN: whether lost AH hits are restored on regain.
void forfeitRegainBonusHits(Ctx& ctx, Character& ch, Guild guild, i32 levelsGained);

// ---- R18 Guild join cost (archive) ----
// R18 the first guild after Nomad is free. UNKNOWN: whether Nomad counts as a joined guild.
bool firstGuildJoinIsFree(const Character& ch);
// R18 Cost = CInt((CCur((Each non-zero guild ability + 1) * 1.25) + CCur(spell class max / 3))^2 * 5356).
// CInt is roundHalfEven. UNKNOWN: how "each non-zero guild ability" and spell class maximum are formed.
i64 guildJoinCost(const Arr<f32, 6>& abilityRatings, i32 spellClassMaxLevel);
// R18 CCur: round after the fourth decimal (1/3 becomes 0.3333)
double ccurRound4(double value);

// ---- R19 Starting age and age display ----
// (ageYears and describeDays are in the API index. Constants are declared above.)
// Starting age is kAgeStartDays in state_character.h.

// ---- R20 Withering (trap 10) ----
// R20 AgeBy(days, party): adds days to the character's age and prints the aging message.
// partyMany is party.Count > 1. (withering(ctx, ch) is in the API index and calls this with kAgeDaysWithering.)
void ageBy(Ctx& ctx, Character& ch, i32 days, bool partyMany);

// ---- R21 Ailment tick (combat-audit) ----
// R21 true when at least kAilmentTickMs have passed since the last tick (explicit ms times).
bool ailmentTickDue(i64 nowMs, i64 lastTickMs);
// R21 poison roll (draw 0..199 + 2*(effectiveCon-10) - 2*poisonLevel), then disease roll, then old-age roll.
// Paralysis uses a separate roll and is not part of this routine.
// FIXED: AilmentTickResult does not exist in any header; replaced with i32 (closest plain type).
i32 ailmentTick(Ctx& ctx, Character& ch, i32 effectiveCon, double agePercent);
// R21 old-age roll threshold: 5 + ln(max(agePercent - 98, 1e-9)). Used when agePercent >= 98.
// UNKNOWN: comparison direction against the 0..99 roll.
double oldAgeRollThreshold(double agePercent);

// R22 Aging wiring: no routine declared. Engine Withering uses ageBy; other wiring is UNKNOWN.

// ---- R23 Arrival rest (engine RestOnArrival) ----
// R23 places the party at the leader's dungeon level and square, clears each member's Area,
// calls restInCity() and Effects.Recompute() per member, and reports nonzero rest days.
// Returns member 0's rest days.
// CONFLICT: arrival healing. Combat-audit says heal and purge; engine says nobody is healed.
// Original rests dead members too. Port skips dead members (port deviation).
// UNKNOWN: owner module of Effects.Recompute (not in index).
i32 restOnArrival(Ctx& ctx, Party& party);

// ---- R24 Rest age cost ----
// R24 ageCostDays = B((100 - 100*currentHP/maxHP) / 3.25). B taken as round-to-nearest (UNKNOWN in SRD).
// Full HP gives 0 days; zero HP gives 31 days. CONFLICT: wiki gives 32 at 100% and 16 at 50%; engine formula used.
i32 restAgeCostDays(i32 curHits, i32 maxHits);

// ---- R25 Failed raise (complications) ----
// R25 chance: complicationChance() in the API index.
// R25 effects: complication() in the API index.
// Constants kComplicationStatLoss, kComplicationMaxHpFactor and kAgeYearsComplication are above.

// ---- R26 Successful raise (CON loss) ----
// R26 ConLoss% = max(0, 50 - 2*CON), floored at 0.
i32 conLossChancePercent(i32 con);
// R26 rolls the CON-loss chance using ctx.rng.percent. Returns true if the loss applies.
// UNKNOWN: amount of CON lost; the caller applies it.
bool rollRaiseConLoss(Ctx& ctx, i32 con);

// ---- R27 Play-time accumulator (sub16F8) ----
// R27 if not paused, add (now - lastTick) to play time; always set lastTick = now.
// Verifier: the test sequence gives 15 seconds, not 10, because lastTick refreshes on every tick.
// FIXED: PlayClockStep does not exist in any header; replaced with i64 (the elapsed ms added to play time).
i64 playClockStep(i64 nowMs, i64 lastTickMs, bool paused);

}  // namespace mordor
