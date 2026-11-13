// status2.cpp: Chapter 8 status effects (SRD c08p2).
// Sources: preserved formulas (poison, disease, paralysis), Tables A to L, and the
// quirk and conflict notes in the SRD part. The SRD part has no R-numbers, so
// citations use the table letters and the c-refs from the notes.
#include "mordor/status2.h"

#include "mordor/types.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/companions1b.h"  // stoneChance(con, levelFound) is implemented there

namespace mordor {

namespace {

// Paralysis and stone save floor. CONFLICT: the archive gives a 5% minimum
// (c99, Table K "5..90"), and the engine caps at 95 (c14). Both are applied here.
constexpr i32 kParalysisChanceFloor = 5;
constexpr i32 kStoneChanceFloor = 5;
// UNKNOWN: paralysis hold duration in rounds is not supplied in the SRD part.
constexpr i32 kParalysisHoldRounds = 1;
// Port's help cry threshold: curHits below maxHits / 4. UNKNOWN: original threshold.
constexpr i32 kHelpCryLowDivisor = 4;
// Port ambient sound: flat 10% per tick when nothing is in front (c45). UNKNOWN original.
constexpr i32 kAmbientSoundPercent = 10;

Character& memberAt(Party& party, i32 index) {
    return party.members[static_cast<u32>(index)];
}

const Character& memberAt(const Party& party, i32 index) {
    return party.members[static_cast<u32>(index)];
}

// Shared affliction roll for poison and disease (c08p2 preserved formulas):
// Rnd(100)+1 > Res(c,index) and level field == 0 -> level = levelFound.
// The roll is drawn even when the level test fails, because VB "And" does not short-circuit.
bool afflictByResist(Ctx& ctx, const Character& c, i32 resistIndex, i32& levelField,
                     i32 levelFound, const char* code, const char* text) {
    const i32 roll = ctx.rng.below(100) + 1;
    const bool afflicted = roll > resistPercentAt(c, resistIndex) && levelField == 0;
    if (afflicted) {
        levelField = levelFound;
        ctx.say(code, text, {static_cast<i64>(levelFound)}, {});
    }
    return afflicted;
}

// Band test used by breath, spit and electrocution (Table D).
// Above 1.25 * ref -> high, below 0.75 * ref -> low, otherwise mid.
i32 bandPercent(i32 victimLevel, i32 refLevel, i32 high, i32 mid, i32 low) {
    const double v = static_cast<double>(victimLevel);
    const double m = static_cast<double>(refLevel);
    if (v > kBandHighRatio * m) {
        return high;
    }
    if (v < kBandLowRatio * m) {
        return low;
    }
    return mid;
}

// Rerolls a value 1..max (Table F/chest fate). Returns 0 when max is not positive.
i32 rerollOneTo(Ctx& ctx, i32 max) {
    if (max <= 0) {
        return 0;
    }
    return ctx.rng.below(max) + 1;
}

const char* deathCauseText(DeathCause cause) {
    switch (cause) {
        case DeathCause::Rock:
            return "has been turned to rock.";
        case DeathCause::Drowning:
            return "has drowned.";
        case DeathCause::OldAge:
            return "died of natural causes.";
        case DeathCause::Poison:
            return "has died of poison.";
        case DeathCause::Disease:
            return "has died of disease.";
        case DeathCause::Unspecified:
        default:
            return "has died.";
    }
}

// Sets the death state shared by every death path (no message).
void setDeathState(Character& c, DeathState state) {
    c.deathState = state;
    c.vitals.curHits = 0;
    c.deaths += 1;
}

}  // namespace

// ---------------------------------------------------------------------------
// Resistances and affliction (c08p2 preserved formulas, Table B)
// ---------------------------------------------------------------------------

i32 resistPercentAt(const Character& c, i32 index) {
    if (index < 0 || index >= kResistKinds) {
        return 0;
    }
    return c.resistances.resist[static_cast<u32>(index)];
}

bool tryPoison(Ctx& ctx, Character& c, i32 levelFound) {
    // Preserved formula: Rnd(100)+1 > Res(c,5) and PoisonLevel == 0 -> PoisonLevel = LevelFound.
    return afflictByResist(ctx, c, kResistPoison, c.ailments.poisonLevel, levelFound,
                           "status.poisoned", "You have been poisoned!");
}

bool tryDisease(Ctx& ctx, Character& c, i32 levelFound) {
    // Preserved formula: Rnd(100)+1 > Res(c,4) and DiseaseLevel == 0 -> DiseaseLevel = LevelFound.
    return afflictByResist(ctx, c, kResistDisease, c.ailments.diseaseLevel, levelFound,
                           "status.diseased", "You have caught a disease!");
}

// ---------------------------------------------------------------------------
// Paralysis (c08p2 preserved formula, Table K, quirks list)
// ---------------------------------------------------------------------------

i32 paralysisChance(i32 resistPercent, i32 con) {
    // Preserved: chance = min(Res(c,8)+CON, 95).
    // CONFLICT: lower floor 5 (Table K, c99) is applied as well; see kParalysisChanceFloor.
    return clampv(resistPercent + con, kParalysisChanceFloor, kParalysisChanceCap);
}

bool tryParalysis(Ctx& ctx, Character& c, i32 monsterLevel, i32 effCon) {
    // Preserved: if Rnd(100)+1 > chance and ParalyzedRounds == 0 then Paralyse(monLvl).
    const i32 chance = paralysisChance(resistPercentAt(c, kResistParalysis), effCon);
    const i32 roll = ctx.rng.below(100) + 1;
    if (roll > chance && c.ailments.paralyzedRounds == 0) {
        paralyse(ctx, c, monsterLevel);
        return true;
    }
    return false;
}

void paralyse(Ctx& ctx, Character& c, i32 monsterLevel) {
    c.ailments.paralysisLevel = monsterLevel;
    if (monsterLevel > 0) {
        c.ailments.paralysed = true;
        // UNKNOWN: hold duration not supplied; one round used.
        c.ailments.paralyzedRounds = kParalysisHoldRounds;
    }
    // Level-0 paralysis holds nobody (c2, c30). UNKNOWN: the size of the stat-part strip
    // (c2, c30) is not supplied, so the stats are not changed here.
    ctx.say("status.paralysed", "You are paralysed!", {static_cast<i64>(monsterLevel)}, {});
}

void unparalyse(Ctx& ctx, Character& c) {
    // UNKNOWN: the original restores the stat part stripped by paralyse (c2). The amount is not
    // supplied, so the stats are not changed here. ParalysisLevel is left alone (see
    // clearParalysisAtCityGate).
    c.ailments.paralysed = false;
    c.ailments.paralyzedRounds = 0;
    ctx.say("status.unparalysed", "You can move again.", {}, {});
}

void clearParalysisAtCityGate(Ctx& ctx, Character& c) {
    // Reference (engine_B_0066_124): the city gate clears the paralysed state.
    // The stat part stays lost (c2, c30, c34); it is not restored here.
    // CONFLICT 6: the original path is only partly supplied; the flag clear follows the reference test.
    c.ailments.paralysed = false;
    c.ailments.paralysisLevel = 0;
    c.ailments.paralyzedRounds = 0;
    ctx.say("status.paralysisCleared", "The paralysis leaves you at the city gate.", {}, {});
}

bool paralysisWearOff(Ctx& ctx, Character& c, i32 hold, bool monstersOnSquare) {
    if (!c.ailments.paralysed) {
        return false;
    }
    // CONFLICT 4: wear-off ends at once when nothing is on the square (c18, c45).
    if (!monstersOnSquare) {
        unparalyse(ctx, c);
        return true;
    }
    // UNKNOWN: the meaning of "hold" is not explained (subtracted and used as the end chance).
    // Implemented as a countdown: hold is subtracted from ParalyzedRounds.
    c.ailments.paralyzedRounds -= hold;
    if (c.ailments.paralyzedRounds <= 0) {
        unparalyse(ctx, c);
        return true;
    }
    return false;
}

i32 tickSaveClamp(i32 chance) {
    // Table K: monster tick save clamped to 5..90.
    return clampv(chance, kTickSaveMin, kTickSaveMax);
}

bool canTakeStairs(const Character& c, bool goingUp) {
    // Quirk (MAINMOD2.BAS:3953, c30): paralysis bars going up, not going down.
    if (goingUp && c.ailments.paralysed) {
        return false;
    }
    return true;
}

bool castCureParalysis(Ctx& ctx, Character& caster, bool inFight) {
    // CONFLICT 7: the fail-in-fight condition appears only in c31; the frees-the-caster
    // behaviour is from c30 and c34 (the wiki's "target-only" note is not followed).
    if (inFight) {
        ctx.say("status.cureParalysisFailed", "The spell fails in the heat of combat.", {}, {});
        return false;
    }
    unparalyse(ctx, caster);
    return true;
}

void endEveryAilment(Ctx& ctx, Character& c) {
    // R45 EndEveryAilment: Unparalyse; CurePoison; CureDisease; SpellResist[all] = 0; Effects.Recompute
    // CONFLICT 3: c2 clears both levels (followed here); c34 says the port leaves them.
    unparalyse(ctx, c);
    c.ailments = Ailments{};   // CurePoison, CureDisease (flags and levels)
    // FIXED: Character has no spellResist member; the spell resistance values are the
    // resistances.resist array (the same array read by resistPercentAt).
    for (auto& r : c.resistances.resist) r = 0;
    // TODO: Effects.Recompute (R45) once its C++ entry point is confirmed
    ctx.say("status.ailmentsEnded", "Your ailments end.", {}, {});
}

// ---------------------------------------------------------------------------
// Poison and disease cures and ticks (Table F, quirks, CONFLICT 1 to 2)
// ---------------------------------------------------------------------------

i32 cureRollBonus(i32 effCon, i32 ailmentLevel) {
    // Table F: + (effCON - 10) * 2 - ailmentLevel * 2.
    return (effCon - kCureConBase) * kCureConFactor - ailmentLevel * kCureLevelFactor;
}

bool rollPoisonCure(Ctx& ctx, i32 effCon, i32 poisonLevel) {
    // Roll = Int(Rnd*200) + cureRollBonus. Cured when roll > 185.
    const i32 roll = ctx.rng.below(kCureRollSides) + cureRollBonus(effCon, poisonLevel);
    return roll > kPoisonCureRollAbove;
}

bool rollDiseaseCure(Ctx& ctx, i32 effCon, i32 poisonLevel) {
    // Quirk (Table F, c29): the disease cure reads the POISON level, not the disease level.
    // Roll = Int(Rnd*200) + (effCON-10)*2 - poisonLevel*2. Cured when roll > 195.
    const i32 roll = ctx.rng.below(kCureRollSides) + cureRollBonus(effCon, poisonLevel);
    return roll > kDiseaseCureRollAbove;
}

bool poisonTick(Ctx& ctx, Party& party, i32 memberIndex, i32 effCon) {
    Character& c = memberAt(party, memberIndex);
    if (c.ailments.poisonLevel == 0 || c.isDead()) {
        return false;
    }
    if (rollPoisonCure(ctx, effCon, c.ailments.poisonLevel)) {
        c.ailments.poisonLevel = 0;
        ctx.say("status.poisonCured", "The poison wears off.", {}, {});
        return true;  // cured this tick (passed to diseaseTick as curedPoison)
    }
    c.vitals.curHits -= kPoisonHitsPerFail;
    ctx.say("status.poisonDamage", "The poison hurts you.",
            {static_cast<i64>(kPoisonHitsPerFail), static_cast<i64>(c.vitals.curHits)}, {});
    if (c.vitals.curHits <= 0) {
        killCharacter(ctx, party, memberIndex, DeathState::Dead, DeathCause::Poison, true);
    }
    return false;
}

bool diseaseTick(Ctx& ctx, Party& party, i32 memberIndex, i32 effCon, bool curedPoison) {
    Character& c = memberAt(party, memberIndex);
    if (c.ailments.diseaseLevel == 0 || c.isDead()) {
        return false;
    }
    // CONFLICT 2: c19 and c30 subtract 0 when the poison was cured on this tick (curedPoison),
    // otherwise poisonLevel * 2. c34 disagrees and is treated as inconsistent.
    const i32 poisonTerm = curedPoison ? 0 : c.ailments.poisonLevel;
    if (rollDiseaseCure(ctx, effCon, poisonTerm)) {
        c.ailments.diseaseLevel = 0;
        ctx.say("status.diseaseCured", "The disease clears.", {}, {});
        return true;
    }
    c.vitals.curHits -= kDiseaseHitsPerFail;
    ctx.say("status.diseaseDamage", "The disease wastes you.",
            {static_cast<i64>(kDiseaseHitsPerFail), static_cast<i64>(c.vitals.curHits)}, {});
    if (c.vitals.curHits <= 0) {
        killCharacter(ctx, party, memberIndex, DeathState::Dead, DeathCause::Disease, true);
    }
    return false;
}

// ---------------------------------------------------------------------------
// Death paths (Table J, c30, c32, c34, c44, c45)
// ---------------------------------------------------------------------------

void killCharacter(Ctx& ctx, Party& party, i32 victimIndex, DeathState state, DeathCause cause,
                   bool runDeathRoutine) {
    Character& c = memberAt(party, victimIndex);
    setDeathState(c, state);
    ctx.say("status.killed", deathCauseText(cause), {static_cast<i64>(victimIndex)}, {c.name});
    // Quirk (c2, c32): callers that pass runDeathRoutine == false skip the ailment-clearing head.
    if (runDeathRoutine) {
        deathRoutine(ctx, party, victimIndex);
    }
}

void deathRoutine(Ctx& ctx, Party& party, i32 victimIndex) {
    deathHeadRoutine(ctx, party, victimIndex);
    if (deathOffersRescue(party)) {
        party.awaitingRescue = true;
        ctx.say("status.rescueOffered", "Someone may be able to find you.", {}, {});
    }
}

void deathHeadRoutine(Ctx& ctx, Party& party, i32 victimIndex) {
    // Ailment-clearing head (c2, c32): the dead member's ailments end.
    endEveryAilment(ctx, memberAt(party, victimIndex));
}

bool deathOffersRescue(const Party& party) {
    // Port quirk (c45): the "UBound > 0" test means a party death in the party window (size 1)
    // does not offer rescue. UNKNOWN: whether the original behaves the same.
    return party.size > 1;
}

bool helpCryNeeded(const Party& party, const Character& c) {
    // HelpCry.Needed (c45): party size > 1 and HP low. UNKNOWN: low threshold, quarter of max used.
    if (party.size <= 1 || c.isDead()) {
        return false;
    }
    return c.vitals.curHits * kHelpCryLowDivisor < c.vitals.maxHits;
}

void drownCharacter(Ctx& ctx, Party& party, i32 victimIndex) {
    // Quirk (c34, c30): drowning kills silently; the only text is the square label, which is
    // shown elsewhere. The death routine is not run here.
    // RESOLVED-BY-REFERENCE: the ailment head runs at every death. Character.cs,
    // EndAilmentsAndSpellResists: "sub048C ... runs at the city gate (FRMCTYM.BAS:130) and at every
    // death (sub0930, MAINMOD2.BAS:3033)". Character.cs, Sub0930Head: "sub048C ends paralysis,
    // poison, disease and the spell resistances". Drowning is a death, so paralysis, poison and
    // disease are cleared here. The spell resistances have no field in this file and are not
    // cleared. The companion and carry parts of the death routine are not run here.
    Character& victim = memberAt(party, victimIndex);
    victim.ailments.paralysed = false;
    victim.ailments.paralyzedRounds = 0;
    victim.ailments.poisonLevel = 0;
    victim.ailments.diseaseLevel = 0;
    setDeathState(victim, DeathState::Dead);
    (void)ctx;
}

void rockDeath(Ctx& ctx, Party& party, i32 victimIndex) {
    // Rock death (cause "rock"). UNKNOWN: the DeathRock value is not supplied; the enum value
    // DeathState::Rock from state_common.h is used.
    killCharacter(ctx, party, victimIndex, DeathState::Rock, DeathCause::Rock, true);
}

// ---------------------------------------------------------------------------
// Stoning, old age, ambient sound
// ---------------------------------------------------------------------------

bool rollStoning(Ctx& ctx, const Character& c, i32 levelFound) {
    // Table K: stone cap 95, floor 5 (CONFLICT 14). Chance comes from stoneChance (companions1b).
    const i32 con = c.abilities.stats[static_cast<u32>(StatIndex::Con)];
    const double chance = clampv(stoneChance(con, levelFound),
                                 static_cast<double>(kStoneChanceFloor),
                                 static_cast<double>(kStoneChanceCap));
    // c23: a character is stoned when both saves fail. The first save uses the chance above, the
    // second uses the stone resistance (index 7, Table B). Both rolls are drawn.
    // CONFLICT 15: comparator direction for the first save follows c64 (roll > value). The exact
    // form in c14 is UNKNOWN.
    const i32 roll1 = ctx.rng.below(100) + 1;
    const i32 roll2 = ctx.rng.below(100) + 1;
    const bool save1Fails = static_cast<double>(roll1) > chance;
    const bool save2Fails = roll2 > resistPercentAt(c, kResistStone);
    return save1Fails && save2Fails;
}

bool rollDiesOfOldAge(Ctx& ctx, const Character& c) {
    // UNKNOWN: Rules.DiesOfOldAge formula not supplied (c45). Placeholder: no death from age.
    (void)ctx;
    (void)c;
    return false;
}

bool rollAmbientSound(Ctx& ctx, bool monsterInFront) {
    // Port (c45): flat 10% per tick when nothing is in front. UNKNOWN: original behaviour.
    if (monsterInFront) {
        return false;
    }
    if (ctx.rng.percent(kAmbientSoundPercent)) {
        ctx.say("status.ambientSound", "You hear a distant sound.", {}, {});
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Monster special attacks (Tables C and D)
// ---------------------------------------------------------------------------

bool hasSpecialBit(u32 flags, SpecialBit bit) {
    return ((flags >> static_cast<u32>(bit)) & 1u) != 0u;
}

i32 breathPercent(i32 victimLevel, i32 monsterGuildLevel) {
    // Table D (audit c23): >1.25M -> 80, 0.75M..1.25M -> 50, <0.75M -> 20.
    // CONFLICT 8: archive c99 uses a different lower band edge; c23 is followed.
    return bandPercent(victimLevel, monsterGuildLevel,
                       kBreathHighPercent, kBreathMidPercent, kBreathLowPercent);
}

i32 spitPercent(i32 victimLevel, i32 monsterGuildLevel) {
    // Table D (audit c23): >1.25M -> 90, 0.75M..1.25M -> 50, <0.75M -> 10.
    // CONFLICT 17: branch mapping is inferred by position.
    return bandPercent(victimLevel, monsterGuildLevel,
                       kSpitHighPercent, kSpitMidPercent, kSpitLowPercent);
}

i32 electrocutionPercent(i32 victimLevel, i32 monsterLevel) {
    // c14: "90/50/10% by level". UNKNOWN: which level is used (CONFLICT 16).
    // The monster level parameter is used as the reference level here, with the spit values.
    return bandPercent(victimLevel, monsterLevel,
                       kSpitHighPercent, kSpitMidPercent, kSpitLowPercent);
}

i32 breathResistIndex(BreathType type) {
    switch (type) {
        case BreathType::Fire:
            return kResistFire;
        case BreathType::Cold:
            return kResistCold;
        case BreathType::Acid:
            // UNKNOWN: acid resistance index not supplied (Table B). -1 means "none".
            return -1;
        case BreathType::Electrical:
            // UNKNOWN: electrical resistance index not supplied (Table B). -1 means "none".
            return -1;
        default:
            return -1;
    }
}

// ---------------------------------------------------------------------------
// Aging (c23, Table K)
// ---------------------------------------------------------------------------

i32 rollAgingDays(Ctx& ctx, i32 monsterGuildLevel) {
    // c23: min(1d1825, 365 * clamp(B(M/50), 1, 5)). CONFLICT 10: c99 gives a different cap.
    // UNKNOWN: the B() symbol is not defined; the identity is used here.
    const i32 die = ctx.rng.below(kMaxAgingDays) + 1;  // 1d1825 = Int(Rnd*1825)+1
    const i32 scale = clampv(monsterGuildLevel / kAgingMonsterDivisor, kAgingClampMin, kAgingClampMax);
    const i32 cap = kAgingDaysPerYear * scale;
    return minv(die, cap);
}

// ---------------------------------------------------------------------------
// Drain (Table E, quirks, CONFLICT 11 and 12)
// ---------------------------------------------------------------------------

bool rollDrainSave(Ctx& ctx, const Character& c) {
    // Drain save uses resistance index 9 (Table B). UNKNOWN: comparator not stated; the poison
    // convention (roll > Res means affected) is assumed, so this returns true when resisted.
    const i32 roll = ctx.rng.below(100) + 1;
    return roll <= resistPercentAt(c, kResistDrain);
}

DrainPick drainPickFromDraw(i32 draw) {
    // Table E: 1..5 drain stat index draw by 1; 6 drains MaxHits by 2.
    // Quirk: Strength (index 0) is unreachable because the draw starts at 1.
    // Any other draw (0 or out of range) drains nothing (amount 0).
    DrainPick pick{};
    if (draw >= 1 && draw <= 5) {
        pick.stat = static_cast<StatIndex>(draw);
        pick.amount = -1;
    } else if (draw == 6) {
        pick.hitsDrain = true;
        pick.amount = -2;
    }
    return pick;
}

DrainPick rollDrain(Ctx& ctx) {
    // Table E: Int(Rnd*6+1).
    return drainPickFromDraw(ctx.rng.below(6) + 1);
}

void applyDrain(Ctx& ctx, Character& c, DrainPick pick) {
    if (pick.amount == 0) {
        return;
    }
    if (pick.hitsDrain) {
        c.vitals.maxHits += pick.amount;
        if (c.vitals.curHits > c.vitals.maxHits) {
            c.vitals.curHits = c.vitals.maxHits;
        }
        ctx.say("status.maxHitsDrained", "Your maximum hits are drained.",
                {static_cast<i64>(pick.amount), static_cast<i64>(c.vitals.maxHits)}, {});
        return;
    }
    // CONFLICT 12: the race floor for stat drain is not applied (floors unverified).
    const u32 idx = static_cast<u32>(static_cast<i32>(pick.stat));
    c.abilities.stats[idx] += pick.amount;
    ctx.say("status.statDrained", "You feel drained.",
            {static_cast<i64>(idx), static_cast<i64>(pick.amount)}, {});
}

// ---------------------------------------------------------------------------
// Chest traps (Table G)
// ---------------------------------------------------------------------------

void chestFate(Ctx& ctx, Character& c) {
    // Trap 6: rerolls current HP and SP, 1..max.
    c.vitals.curHits = rerollOneTo(ctx, c.vitals.maxHits);
    c.vitals.curSpellPoints = rerollOneTo(ctx, c.vitals.maxSpellPoints);
    ctx.say("status.fate", "Fate reshuffles your strength.",
            {static_cast<i64>(c.vitals.curHits), static_cast<i64>(c.vitals.curSpellPoints)}, {});
}

void clearSpellEffects(Ctx& ctx, Character& c) {
    // Trap 8 (Blackout): clears all spell effects.
    c.ailments.levitating = false;
    c.ailments.invisible = false;
    c.ailments.seeInvisible = false;
    c.ailments.protectLevel = 0;
    c.activeEffectIds.clear();
    ctx.say("status.blackout", "Everything goes dark.", {}, {});
}

bool chestPoisonGas(Ctx& ctx, Character& c, i32 dungeonLevel) {
    // Trap 1 (Poison Gas): afflicted when Rnd(100)+1 is under half the resistance (Res 5),
    // so higher resistance means more affliction. Level = dungeonLevel. No message (quirk, c4).
    const i32 roll = ctx.rng.below(100) + 1;
    const bool afflicted = roll * 2 < resistPercentAt(c, kResistPoison) && c.ailments.poisonLevel == 0;
    if (afflicted) {
        c.ailments.poisonLevel = dungeonLevel;
    }
    return afflicted;
}

bool chestDiseaseSpores(Ctx& ctx, Character& c, i32 dungeonLevel) {
    // Trap 2 (Disease Spores): same rule with resistance 4. No message (quirk, c4).
    const i32 roll = ctx.rng.below(100) + 1;
    const bool afflicted = roll * 2 < resistPercentAt(c, kResistDisease) && c.ailments.diseaseLevel == 0;
    if (afflicted) {
        c.ailments.diseaseLevel = dungeonLevel;
    }
    return afflicted;
}

// ---------------------------------------------------------------------------
// Cure spells and town effects (Table H, c2)
// ---------------------------------------------------------------------------

i32 diseasedHealing(const Character& c, i32 healAmount) {
    // c2 (single source): healing is halved while diseased. The rounding is floor (VbInt).
    // UNKNOWN: VbInt vs CInt rounding (see Unknowns).
    if (c.ailments.diseaseLevel > 0) {
        return static_cast<i32>(vbInt(static_cast<double>(healAmount) / 2.0));
    }
    return healAmount;
}

void applyCureSpell(Ctx& ctx, Character& target, CureSpellKind kind) {
    switch (kind) {
        case CureSpellKind::RemovePoison:
            target.ailments.poisonLevel = 0;
            ctx.say("status.spellRemovePoison", "The poison is removed.", {}, {});
            break;
        case CureSpellKind::RemoveDisease:
            target.ailments.diseaseLevel = 0;
            ctx.say("status.spellRemoveDisease", "The disease is removed.", {}, {});
            break;
        case CureSpellKind::Cure: {
            // UNKNOWN: heal amount not supplied (Table H). The missing hits are healed here,
            // halved while diseased. The heal is applied before the disease is cleared.
            const i32 missing = target.vitals.maxHits - target.vitals.curHits;
            if (missing > 0) {
                target.vitals.curHits += diseasedHealing(target, missing);
            }
            target.ailments.poisonLevel = 0;
            target.ailments.diseaseLevel = 0;
            ctx.say("status.spellCure", "You feel better.", {}, {});
            break;
        }
        default:
            break;
    }
}

void clearPoisonInTown(Ctx& ctx, Character& c) {
    // c2 (single source): resting in town cures poison. UNKNOWN: whether the original also clears
    // the level or only the flag; the level is cleared here.
    if (c.ailments.poisonLevel == 0) {
        return;
    }
    c.ailments.poisonLevel = 0;
    ctx.say("status.townPoisonCleared", "The poison leaves you in town.", {}, {});
}

// ---------------------------------------------------------------------------
// Death-effect spell names (Table I)
// ---------------------------------------------------------------------------

DeathSpellEffect deathSpellEffectByName(const Str& spellName) {
    for (const char* name : kKilledSpellNames) {
        if (spellName == name) {
            return DeathSpellEffect::Killed;
        }
    }
    for (const char* name : kDrainedSpellNames) {
        if (spellName == name) {
            return DeathSpellEffect::Drained;
        }
    }
    return DeathSpellEffect::None;
}

}  // namespace mordor
