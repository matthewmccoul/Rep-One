#pragma once
// Status effects: poison, disease, paralysis, stoning, drain, death routines, chest traps, cures.
// SRD part c08p2 gives no R-numbers, so cites use its source tags [cNN].
// Not declared here (no engine mechanics in this part): status flag bits (Table L, design guide only),
// curse, sleep, fear, invisibility, item destruction (all UNKNOWN in the SRD).

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_common.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"
#include "mordor/state_party.h"

namespace mordor {

// ---- Constants and tables ----

// Four-second ailment tick interval [c45]
inline constexpr i32 kAilmentTickMs = 4000;

// Ailment store indices (Table A) [c11][c68]
enum class AilmentSlot : i32 {
    Poison = 0,
    Disease = 1,
    Paralysis = 2,
    Levitating = 3,
    Invisible = 4,
    SeeInvisible = 5,
    Protected = 6
};

// Resistance indices used in this part (Table B) [c14][c4][c23][c20]
// Acid and Electrical indices are UNKNOWN and deliberately not declared.
inline constexpr i32 kResistFire = 0;
inline constexpr i32 kResistCold = 1;
inline constexpr i32 kResistDisease = 4;
inline constexpr i32 kResistPoison = 5;
inline constexpr i32 kResistStone = 7;
inline constexpr i32 kResistParalysis = 8;
inline constexpr i32 kResistDrain = 9;

// Caps and clamps [c18][c23] (Table K)
inline constexpr i32 kParalysisChanceCap = 95;
inline constexpr i32 kStoneChanceCap = 95;
inline constexpr i32 kTickSaveMin = 5;   // monster tick save clamp
inline constexpr i32 kTickSaveMax = 90;

// Poison and disease cure (Table F) [c19][c25][c29][c45]
inline constexpr i32 kPoisonCureRollAbove = 185;
inline constexpr i32 kDiseaseCureRollAbove = 195;
inline constexpr i32 kCureRollSides = 200;          // Int(Rnd*200)
inline constexpr i32 kCureConBase = 10;             // (effCON-10)*2
inline constexpr i32 kCureConFactor = 2;
inline constexpr i32 kCureLevelFactor = 2;          // -ailmentLevel*2
inline constexpr i32 kPoisonHitsPerFail = 2;
inline constexpr i32 kDiseaseHitsPerFail = 1;

// Breath and spit bands (Table D) [c23]
// CONFLICT: archive c99 uses a different lower band edge (1.25M); resolution is c23.
inline constexpr double kBandHighRatio = 1.25;      // victim level > 1.25 * monster level
inline constexpr double kBandLowRatio = 0.75;       // victim level < 0.75 * monster level
inline constexpr i32 kBreathHighPercent = 80;
inline constexpr i32 kBreathMidPercent = 50;
inline constexpr i32 kBreathLowPercent = 20;
inline constexpr i32 kSpitHighPercent = 90;
inline constexpr i32 kSpitMidPercent = 50;
inline constexpr i32 kSpitLowPercent = 10;

// Monster aging (c23) [c23][c18]
// CONFLICT: archive c99 uses min(1825, M*7.3); resolution is c23.
inline constexpr i32 kMaxAgingDays = 1825;
inline constexpr i32 kAgingDaysPerYear = 365;
inline constexpr i32 kAgingClampMin = 1;
inline constexpr i32 kAgingClampMax = 5;
inline constexpr i32 kAgingMonsterDivisor = 50;     // B(M/50); B() UNKNOWN, see rollAgingDays

// Death spell names (Table I) [c97]
inline constexpr const char* kKilledSpellNames[6] = {
    "Cause Wounds", "Cause Heavy Wounds", "Power", "Harm", "Cause Fatal Wounds", "Word of Death"};
inline constexpr const char* kDrainedSpellNames[1] = {"Draining Touch"};

// Monster special-attack bits (Table C) [c23]
enum class SpecialBit : i32 {
    Poison = 0,
    Disease = 1,
    Paralysis = 2,
    BreathFire = 3,
    BreathCold = 4,
    Spit = 5,
    Electrocution = 6,
    Drain = 7,
    Petrification = 8,
    Aging = 9,
    Destruction = 12,
    Theft = 13
};

// Cause of death. UNKNOWN: full list not supplied; only Rock is named as DeathRock [c44].
enum class DeathCause : i32 {
    Unspecified = 0,
    Rock = 1,
    Drowning = 2,
    OldAge = 3,
    Poison = 4,
    Disease = 5
};

// Death spell effect (Table I) [c97]
enum class DeathSpellEffect : i32 {
    None = 0,
    Killed = 1,
    Drained = 2
};

// Cure spells that act on a target (Table H) [c89]
// UNKNOWN: Table H "Control" heading is garbled; RemovePoison follows its text.
// Cure Paralysis is handled by castCureParalysis.
enum class CureSpellKind : i32 {
    RemovePoison = 0,
    RemoveDisease = 1,
    Cure = 2                // heals hits (amount UNKNOWN), removes poison and disease
};

// Drain outcome (Table E) [c14][c27][c63]
// hitsDrain true: MaxHits -2 (amount -2). Otherwise stat drain of -1 on stat.
// Strength is unreachable (draw starts at 1). CONFLICT: c84 says strength can be drained; resolution is engine.
struct DrainPick {
    bool hitsDrain = false;
    StatIndex stat = StatIndex::Int;
    i32 amount = 0;
};

// ---- c2 Poison, disease and paralysis rolls ----

// c2 Res(c,i): resistance percent at index i (Table B)
i32 resistPercentAt(const Character& c, i32 index);

// c2 poison: Rnd(100)+1 > Res(c,5) and PoisonLevel == 0 -> PoisonLevel = LevelFound. Returns true if applied.
bool tryPoison(Ctx& ctx, Character& c, i32 levelFound);

// c2 disease: Rnd(100)+1 > Res(c,4) and DiseaseLevel == 0 -> DiseaseLevel = LevelFound. Returns true if applied.
bool tryDisease(Ctx& ctx, Character& c, i32 levelFound);

// c2 paralysis chance: min(Res(c,8)+CON, 95)
i32 paralysisChance(i32 resistPercent, i32 con);

// c2 paralysis: chance = min(Res(c,8)+CON,95); if Rnd(100)+1 > chance and ParalyzedRounds==0, Paralyse(monLvl).
// UNKNOWN: which CON is passed in effCon (base, with mods, or with gear); SRD says only "CON".
bool tryParalysis(Ctx& ctx, Character& c, i32 monsterLevel, i32 effCon);

// c2 sub0D1D set: paralyses c, shifts the stat part of Att/Def into ADShift/StoredADShift.
// monsterLevel 0 strips the stat part without holding anyone; stats are not restored [c2][c30].
void paralyse(Ctx& ctx, Character& c, i32 monsterLevel);

// c2 sub0D1D clear: ends paralysis and restores the stat part.
void unparalyse(Ctx& ctx, Character& c);

// c30 city gate clear of ParalysisLevel. Stat part stays lost (quirk).
// CONFLICT: c2 EndEveryAilment would restore stats via Unparalyse; resolution follows c30/c34 quirk. UNKNOWN for port.
void clearParalysisAtCityGate(Ctx& ctx, Character& c);

// c18/c45 paralysis wear-off. hold is subtracted and used as the end chance.
// Always ends when no monsters are on the square (c18, c45).
// UNKNOWN: exact end-chance formula beyond "hold"; sign reasoning not in source.
bool paralysisWearOff(Ctx& ctx, Character& c, i32 hold, bool monstersOnSquare);

// c23 monster tick save clamp (5..90)
i32 tickSaveClamp(i32 chance);

// c30 paralysed characters may go up stairs? No: barred from going up, not down (MAINMOD2.BAS:3953)
bool canTakeStairs(const Character& c, bool goingUp);

// c30/c34 Cure Paralysis: frees the caster. Fails during a fight (c31 only).
// CONFLICT: c89 calls it target-only; resolution is audit (caster). UNKNOWN: fail-in-fight is single source.
bool castCureParalysis(Ctx& ctx, Character& caster, bool inFight);

// c2 sub048C / EndEveryAilment: clears paralysis, poison, disease and spell resistances, then recomputes.
// Stoned state (DeathState::Stoned) survives this routine [c11][c32].
// UNKNOWN: which "recompute" step runs after clearing.
void endEveryAilment(Ctx& ctx, Character& c);

// ---- c45 Ailment tick (per-member checks; the tick loop itself is ailmentTick in clock.h) ----

// c19/c29 cure roll base: (effCON-10)*2 - ailmentLevel*2. The roll adds Int(Rnd*200) on top.
i32 cureRollBonus(i32 effCon, i32 ailmentLevel);

// c19/c25 poison cure: Int(Rnd*200) + cureRollBonus(effCon, poisonLevel) > 185. Returns true if cured.
bool rollPoisonCure(Ctx& ctx, i32 effCon, i32 poisonLevel);

// c19/c30 disease cure: Int(Rnd*200) + cureRollBonus(effCon, X) > 195.
// Quirk: X is the POISON level, not the disease level. Callers pass 0 when poison was cured this tick [c19][c30].
bool rollDiseaseCure(Ctx& ctx, i32 effCon, i32 poisonLevel);

// c45 per-member poison step. Failing costs kPoisonHitsPerFail hits.
// Kills (via killCharacter) at 0 HP. Returns true if the member died, so the caller removes it.
// Quirk: removal shifts later members into this index [c30][c45].
bool poisonTick(Ctx& ctx, Party& party, i32 memberIndex, i32 effCon);

// c45 per-member disease step. Failing costs kDiseaseHitsPerFail hits.
// curedPoison: poison was cured on this tick, so poison level subtracted is 0 (c19/c30).
// Returns true if the member died.
bool diseaseTick(Ctx& ctx, Party& party, i32 memberIndex, i32 effCon, bool curedPoison);

// ---- c2/c44/c45 Death ----

// c45 Kill: sets HP 0, IsDead, Deaths++, logs, runs deathRoutine unless suppressed.
// CONFLICT: c32 table J says 0 is ordinary death; state_common DeathState::Alive==0. Callers pass DeathState::Dead.
void killCharacter(Ctx& ctx, Party& party, i32 victimIndex, DeathState state, DeathCause cause, bool runDeathRoutine);

// c44 Sub0930: death routine. Party size > 1 prunes the dead and offers no rescue. Alone offers rescue.
// Quirk: port's party path kills from the caster onward and stops [c44].
void deathRoutine(Ctx& ctx, Party& party, i32 victimIndex);

// c2/c44 Sub0930Head: releases followers and ends all ailments on the victim.
// Several death paths skip this head (chest trap, companion mishap, list UPDATE, "has disappeared from view!") [c2][c32].
void deathHeadRoutine(Ctx& ctx, Party& party, i32 victimIndex);

// c44/c45 lone vs party: rescue is offered only when the party is solo.
// UNKNOWN for port: "UBound > 0" means a party death in the party window does not offer rescue (c45).
bool deathOffersRescue(const Party& party);

// c45 CallForHelp fires only for party size > 1 when HP is low (HelpCry.Needed).
// UNKNOWN: the "low HP" threshold and the original's behaviour.
bool helpCryNeeded(const Party& party, const Character& c);

// c34/c30 drowning: kills silently. Only the square label "You are Drowning!" / "You are Sinking!" is shown.
// Port's "{me}drowns." text is invented and not emitted [c34].
void drownCharacter(Ctx& ctx, Party& party, i32 victimIndex);

// c44 DeathRock: death for walking into solid rock. Cause is DeathCause::Rock.
// Body in rock is written to level-0 area on sync (port), not carried [c45].
void rockDeath(Ctx& ctx, Party& party, i32 victimIndex);

// c23 stoning roll using Res(c,7) against levelFound. Returns true if the character is stoned.
// Stoned sets DeathState::Stoned (value 2) [c11][c32].
// CONFLICT/UNKNOWN: exact comparator (c14 garbled); c23 says stoned when both saves fail.
bool rollStoning(Ctx& ctx, const Character& c, i32 levelFound);

// c30/c45 old age death check. UNKNOWN: Rules.DiesOfOldAge formula not supplied.
// Old age still announces "died of natural causes" [c30]. Lone-character body keeps (UNKNOWN wording).
bool rollDiesOfOldAge(Ctx& ctx, const Character& c);

// c45 ambient sound: flat 10% per tick when nothing is in front. UNKNOWN for original.
bool rollAmbientSound(Ctx& ctx, bool monsterInFront);

// ---- c23 Monster special attacks ----

// c23 monster special-attack bit test (Table C)
bool hasSpecialBit(u32 flags, SpecialBit bit);

// c23 breath chance % by victim level vs monster guild level (Table D, audit column)
i32 breathPercent(i32 victimLevel, i32 monsterGuildLevel);

// c23 spit chance % by victim level vs monster guild level (Table D).
// CONFLICT: branch mapping of "50/90/10" is inferred by position [c23].
i32 spitPercent(i32 victimLevel, i32 monsterGuildLevel);

// c14 electrocution chance "90/50/10% by level". UNKNOWN: which level is used (character or monster).
i32 electrocutionPercent(i32 victimLevel, i32 monsterLevel);

// c20/c14 breath resistance index for a breath type. Fire=0, Cold=1 (Cold is resisted by Cold even when Fire is higher, quirk [c20]).
// Returns -1 for Acid and Electrical: UNKNOWN indices.
i32 breathResistIndex(BreathType type);

// c23 aging days from a monster: min(1d1825, 365*clamp(B(M/50),1,5)).
// UNKNOWN: B() rounding (VbInt vs CInt) and the "1d1825" range notation.
i32 rollAgingDays(Ctx& ctx, i32 monsterGuildLevel);

// c14/c23 drain save via Res(c,9). UNKNOWN: exact save comparator.
bool rollDrainSave(Ctx& ctx, const Character& c);

// c14/c27/c63 drain pick from draw Int(Rnd*6+1). Draw 1..5 is stat index 1..5 drain -1; draw 6 is MaxHits -2.
DrainPick drainPickFromDraw(i32 draw);

// Rolls the drain pick: drainPickFromDraw(ctx.rng.below(6)+1). Race floor and guild condition are UNKNOWN [c23].
DrainPick rollDrain(Ctx& ctx);

// c14 applies a drain. Stat drains are not floored by race (UNKNOWN [c23]).
void applyDrain(Ctx& ctx, Character& c, DrainPick pick);

// ---- c4 Chest traps and c89 cures ----

// c4 trap 6 Fate: rerolls current HP and SP, each 1..max.
void chestFate(Ctx& ctx, Character& c);

// c4 trap 8 Blackout: clears all spell effects.
void clearSpellEffects(Ctx& ctx, Character& c);

// c4 trap 1 Poison Gas: afflicted via resistance-5 roll, level = dungeonLevel. Returns true if afflicted.
// Quirk: afflicted when roll is under half resistance, so higher resistance means more affliction. No message [c4].
// UNKNOWN: exact roll form.
bool chestPoisonGas(Ctx& ctx, Character& c, i32 dungeonLevel);

// c4 trap 2 Disease Spores: afflicted via resistance-4 roll, level = dungeonLevel. Same quirk as Poison Gas. Returns true if afflicted.
bool chestDiseaseSpores(Ctx& ctx, Character& c, i32 dungeonLevel);

// c89 cure spell effect on a target (Table H).
void applyCureSpell(Ctx& ctx, Character& target, CureSpellKind kind);

// c2 disease halves healing. UNKNOWN: which healing is affected (single source). Returns the healing received.
i32 diseasedHealing(const Character& c, i32 healAmount);

// c2 poison cured by resting in town (single source). UNKNOWN: whether a raised body is also cleared on city arrival [c32].
void clearPoisonInTown(Ctx& ctx, Character& c);

// c97 death spell classification by spell name (Table I).
DeathSpellEffect deathSpellEffectByName(const Str& spellName);

}  // namespace mordor
