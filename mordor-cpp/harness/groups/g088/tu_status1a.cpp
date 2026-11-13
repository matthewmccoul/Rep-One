// status1a.cpp: status effects (SRD c08p1): poison, disease, paralysis, stone,
// drain, aging, monster special attacks, cures.
// Event text is English; numeric payloads are omitted (nums/strs passed empty).
#include <cmath>

#include "mordor/status1a.h"
#include "mordor/math_util.h"
#include "mordor/combat1b.h"  // FIXED: declares SpecialAttackKind
#include "mordor/combat3.h"

namespace mordor {
namespace {

// R26 resistance indices.
constexpr i32 kResFire = 0;
constexpr i32 kResCold = 1;
constexpr i32 kResDisease = 4;
constexpr i32 kResPoison = 5;
constexpr i32 kResStone = 7;
constexpr i32 kResParalysis = 8;
constexpr i32 kResDrain = 9;

constexpr i32 kBitPoison = 0;       // R25 (no header constant)
constexpr i32 kConWeight = 2;       // R7: (effCON - 10) * 2
constexpr i32 kParalysisCap = 95;   // R12 cap on paralysis save chance
constexpr i32 kArchiveChanceFloor = 5;  // archive stone/paralysis minimum 5%
constexpr i32 kTheftSlotMin = 2;    // theft samples slots 2..40 (R36)
constexpr i32 kTheftSlotCount = 39;

// R27 audit breath thresholds, indexed by RatioBand (High, Even, Low).
constexpr Arr<i32, 3> kBreathThresholdPct{{80, 50, 20}};

i32 clampI(i32 v, i32 lo, i32 hi) { return v < lo ? lo : (v > hi ? hi : v); }
i32 maxI(i32 a, i32 b) { return a > b ? a : b; }
double clampD(double v, double lo, double hi) { return v < lo ? lo : (v > hi ? hi : v); }
double minD(double a, double b) { return a < b ? a : b; }
double maxD(double a, double b) { return a > b ? a : b; }

// Rnd(100)+1 (R9, R12, R19, R31, R35): uniform integer 1..100.
i32 d100(Ctx& ctx) { return static_cast<i32>(ctx.rng.below(100)) + 1; }

i32 baseCon(const Character& c) {
    return c.abilities.stats[static_cast<u32>(StatIndex::Con)];
}

// R7 legend: effCON = base CON plus gear CON.
// CONFLICT: the port passes Con without gear in the tick (SRD note); the
// SRD legend and fidelity doc both include gear CON. Gear CON is used here.
i32 effectiveCon(const Character& c) {
    return baseCon(c) + c.abilities.gearCon;
}

i32 statAt(const Character& c, StatIndex s) {
    return c.abilities.stats[static_cast<u32>(s)];
}

i32 resistAt(const Character& c, i32 idx) {
    return c.resistances.resist[static_cast<u32>(idx)];
}

bool hasBit(u32 flags, i32 bit) {
    return ((flags >> static_cast<u32>(bit)) & 1u) != 0u;
}

i32 highestGuildLevel(const Character& c) {
    i32 best = 0;
    for (u32 i = 0; i < static_cast<u32>(kGuildCount); ++i) {
        best = maxI(best, c.guilds[i].level);
    }
    return best;
}

// Event helper. Payload numbers are not passed (see file header).
void note(Ctx& ctx, const char* code, const char* text) {
    ctx.say(code, text, {}, {});
}

// R13 Paralyse(level). Private: not in the header.
void paralyse(Ctx& ctx, Character& c, i32 level) {
    if (c.ailments.paralysisLevel != 0) return;          // R13.1
    c.combat.adShift = storedADShift(c) - statsADNow(c); // R13.2
    c.ailments.paralysisLevel = level;
    note(ctx, "status1a.paralysed", "Paralysed");
}

// R14 Unparalyse. Private: not in the header.
void unparalyse(Ctx& ctx, Character& c) {
    if (c.ailments.paralysisLevel == 0) return;          // R14.1
    c.combat.adShift = storedADShift(c) + statsADNow(c); // R14.2
    c.ailments.paralysisLevel = 0;
    note(ctx, "status1a.unparalysed", "Paralysis ends");
}

// R9 poison infliction. The draw is always taken (UNKNOWN: VB And does not
// short-circuit, so Rnd is consumed even when PoisonLevel is nonzero).
void inflictPoison(Ctx& ctx, Character& victim, i32 attackerLevel) {
    const bool failed = d100(ctx) > resistAt(victim, kResPoison);
    if (failed && victim.ailments.poisonLevel == 0) {
        // CONFLICT: R28 level conflict; LevelFound is taken as the attacker level passed in.
        victim.ailments.poisonLevel = attackerLevel;
        note(ctx, "status1a.poisoned", "Poisoned");
    }
}

// R9 disease infliction. Same draw convention as poison.
void inflictDisease(Ctx& ctx, Character& victim, i32 attackerLevel) {
    const bool failed = d100(ctx) > resistAt(victim, kResDisease);
    if (failed && victim.ailments.diseaseLevel == 0) {
        // CONFLICT: R28 level conflict; LevelFound is taken as the attacker level passed in.
        victim.ailments.diseaseLevel = attackerLevel;
        note(ctx, "status1a.diseased", "Diseased");
    }
}

}  // namespace

// ---------------------------------------------------------------- held state

// R16: a paralysed character (ParalysisLevel > 0) is held.
bool isHeld(const Character& c) {
    return c.ailments.paralysisLevel > 0;
}

// R16: a held character does nothing on its own action. Companions still fight.
bool canActOnOwnTurn(const Character& c) {
    return !isHeld(c);
}

// R17 / R18: refused actions while held. Stairs down and chute are allowed.
bool heldActionRefused(const Character& c, HeldAction action) {
    if (!isHeld(c)) return false;
    switch (action) {
        case HeldAction::StairsDown:  // R18
        case HeldAction::Chute:       // R18
            return false;
        default:                      // R17
            return true;
    }
}

// ---------------------------------------------------------- poison / disease

// R7 / R8: roll offset (effCON - 10) * 2 - level * 2.
i32 ailmentRollOffset(i32 effCon, i32 level) {
    return (effCon - kAilmentConBase) * kConWeight - level * kAilmentLevelWeight;
}

// R7: one four-second ailment tick. Poison first, then disease.
AilmentTickResult poisonDiseaseTick(Ctx& ctx, Character& c) {
    AilmentTickResult r;
    const i32 con = effectiveCon(c);

    if (c.ailments.poisonLevel > 0) {
        const i32 roll = static_cast<i32>(ctx.rng.below(kAilmentRollRange))
                       + ailmentRollOffset(con, c.ailments.poisonLevel);
        if (roll > kPoisonCureAbove) {
            curePoison(ctx, c);
            r.poisonCured = true;
        } else {
            c.vitals.curHits -= kPoisonTickHits;
            r.hitsLost += kPoisonTickHits;
        }
    }

    if (c.ailments.diseaseLevel > 0) {
        // R7: disease roll subtracts poisonLevel*2 unless poison was cured this tick.
        const i32 poisonTerm = r.poisonCured ? 0 : c.ailments.poisonLevel;
        const i32 roll = static_cast<i32>(ctx.rng.below(kAilmentRollRange))
                       + ailmentRollOffset(con, poisonTerm);
        if (roll > kDiseaseCureAbove) {
            cureDisease(ctx, c);
            r.diseaseCured = true;
        } else {
            c.vitals.curHits -= kDiseaseTickHits;
            r.hitsLost += kDiseaseTickHits;
        }
    }
    return r;
}

// R9: monster melee infliction. Poison first, then disease.
void inflictPoisonAndDisease(Ctx& ctx, Character& victim, i32 attackerLevel) {
    inflictPoison(ctx, victim, attackerLevel);
    inflictDisease(ctx, victim, attackerLevel);
}

// R10 CurePoison.
// CONFLICT: SRD pseudocode clears the level as well as the flag; the port
// (c34) clears only the flag. Preferred behaviour (pseudocode) is used here.
void curePoison(Ctx& ctx, Character& c) {
    c.ailments.poisonLevel = 0;
    note(ctx, "status1a.poison_cured", "Poison cured");
}

// R10 CureDisease. Same conflict as CurePoison.
void cureDisease(Ctx& ctx, Character& c) {
    c.ailments.diseaseLevel = 0;
    note(ctx, "status1a.disease_cured", "Disease cured");
}

// R11: disease halves healing while present.
// UNKNOWN: which healing is affected is unknown; all healing is halved (floor).
i32 healingWhileDiseased(const Character& c, i32 healing) {
    if (c.ailments.diseaseLevel <= 0) return healing;
    return static_cast<i32>(vbInt(static_cast<double>(healing) / 2.0));
}

// R11: poison is cured while resting in town.
void townRestCuresPoison(Ctx& ctx, Character& c) {
    if (c.ailments.poisonLevel > 0) {
        curePoison(ctx, c);
    }
}

// ---------------------------------------------------------------- paralysis

// R12: chance = min(Res(c,8) + CON, 95).
i32 meleeParalysisChance(const Character& c) {
    const i32 v = resistAt(c, kResParalysis) + baseCon(c);
    return v > kParalysisCap ? kParalysisCap : v;
}

// R12: melee paralysis check on a damaging hit.
// UNKNOWN: the draw is always taken (VB And does not short-circuit).
// Returns true when the failed save occurred with no paralysis rounds pending
// (Paralyse may still be a no-op if already held, R13.1).
bool meleeParalysisCheck(Ctx& ctx, Character& c, i32 monsterLevel) {
    const i32 chance = meleeParalysisChance(c);
    const bool failed = d100(ctx) > chance;
    if (failed && c.ailments.paralyzedRounds == 0) {
        paralyse(ctx, c, monsterLevel);
        return true;
    }
    return false;
}

// R19: wear-off chance, percent, clamped to 5..90.
double paralysisWearOffChance(const Character& c, i32 monsterGuildLevel) {
    const i32 pLevel = c.ailments.paralysisLevel < 1 ? 1 : c.ailments.paralysisLevel;
    const double p = static_cast<double>(pLevel);
    const double hold = std::log(p / 2.0 + p / 10.0) / std::log(1.02);

    const i32 lvlInt = monsterGuildLevel < 1 ? 1 : monsterGuildLevel;
    const double lvl = static_cast<double>(lvlInt);
    const double fight =
        ((std::log(lvl) + lvl / 10.0) / std::log(1.05) / 10.0 + baseCon(c) * 1.5)
        + resistAt(c, kResParalysis);

    return clampD(hold - fight, 5.0, 90.0);
}

// R19 (four-second tick, R33 step 2d): wear-off roll. Ends if Int(Rnd*100)+1 > chance.
// CONFLICT: the port gates on _combat != null (c34); the engine gates on a monster
// on the square (c45). The caller is responsible for the gate; see paralysisNoMonsterClear.
bool paralysisWearOffTick(Ctx& ctx, Character& c, i32 monsterGuildLevel) {
    if (!isHeld(c)) return false;
    const double chance = paralysisWearOffChance(c, monsterGuildLevel);
    const i32 roll = d100(ctx);
    if (static_cast<double>(roll) > chance) {
        unparalyse(ctx, c);
        return true;
    }
    return false;
}

// R20: paralysis always ends when no monster is on the square.
bool paralysisNoMonsterClear(Ctx& ctx, Character& c, bool monsterOnSquare) {
    if (monsterOnSquare || !isHeld(c)) return false;
    unparalyse(ctx, c);
    return true;
}

// ------------------------------------------------------------ special attacks

// FIXED: forward declaration; defined below, used by monsterGroupSpecialTurn.
static i32 specialResistIndexOf(SpecialAttackKind kind);

// R22: monster group special effects against one victim (party member).
// UNKNOWN: the group loop, companion slots and melee damage are resolved by
// the caller. Party carries no companion list, so companion breath, companion
// stoning, destruction (no Inventory reachable) and theft are not applied here.
// The melee hit is assumed resolved, so melee side effects are applied.
// UNKNOWN: relative order of melee side effects; list order of R22 item 4 is used.
void monsterGroupSpecialTurn(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex) {
    if (victimIndex < 0 || victimIndex >= party.size) return;
    Character& v = party.members[static_cast<u32>(victimIndex)];
    if (v.isDead()) return;

    const u32 flags = attacker.specialPropertyFlags;
    const i32 monsterLevel = attacker.guildLevel;  // M in R27/R28/R32
    const i32 victimLevel = v.activeLevel();

    // Step 1: breath, bits 3 and 4 (R22.1, R24, R28).
    const bool fire = hasBit(flags, kBitBreathFire);
    const bool cold = hasBit(flags, kBitBreathCold);
    if (fire || cold) {
        // CONFIRMED-BY-REFERENCE: MonsterAttack.cs breath gate `_rng.Next(100) < chance`
        // is Int(Rnd*100) < chance, i.e. d100 <= threshold. Threshold table (50/80/20) is in breathThresholdPercent.
        if (d100(ctx) <= breathThresholdPercent(highestGuildLevel(v), monsterLevel)) {
            // RESOLVED-BY-REFERENCE: MonsterAttack.cs (MONSMOD.BAS:2692-2757):
            // "int l046C = VbInt(monLvl * Math.Log(monLvl + 1));" then "if (l046C <= 0) yield break;".
            // Log(0) raises an error that ends the whole group turn, so acid, specials and the rest
            // of the turn are skipped. Level 0, and level 1 (VbInt(0.69) = 0), take this path.
            const double bm = static_cast<double>(monsterLevel);
            if (vbInt(bm * std::log(bm + 1.0)) <= 0) return;
            i32 dmg = rollBreathDamage(ctx, monsterLevel, victimLevel);
            dmg = applySpecialResistance(dmg, resistAt(v, fire ? kResFire : kResCold));  // R24
            v.vitals.curHits -= dmg;
            note(ctx, "status1a.breath", "Breath strikes");
        }
    }

    // Step 1b: acid spit, bit 5 (R27 spit thresholds on active level, R28).
    if (hasBit(flags, kBitSpit)) {
        // CONFIRMED-BY-REFERENCE: MonsterAttack.cs acid gate `_rng.Next(100) < chance` (50/90/10 by level)
        // is the same roll form as d100 <= threshold. Threshold body is not in this input.
        if (d100(ctx) <= spitThresholdPercent(v.activeLevel(), monsterLevel)) {
            i32 dmg = rollSpitDamage(ctx, monsterLevel, victimLevel);
            // FIXED: header declares an i32 named specialResistIndex; function renamed.
            dmg = applySpecialResistance(
                dmg, resistAt(v, specialResistIndexOf(SpecialAttackKind::AcidSpit)));
            v.vitals.curHits -= dmg;
            note(ctx, "status1a.spit", "Acid spits");
        }
    }

    // Step 3: electrocution, bit 6, checked against the hit target (R29).
    // UNKNOWN: electrocution damage formula is not in the input; only the chance
    // is rolled and reported.
    if (hasBit(flags, kBitElectrocution)) {
        if (d100(ctx) <= electrocutionChancePercent(v.level, monsterLevel)) {
            note(ctx, "status1a.electrocuted", "Electrocution");
        }
    }

    // Step 4: melee side effects (R22.4), list order.
    if (hasBit(flags, kBitPoison)) {
        inflictPoison(ctx, v, attacker.levelFound);
    }
    if (hasBit(flags, kBitDisease)) {
        inflictDisease(ctx, v, attacker.levelFound);
    }
    if (hasBit(flags, kBitParalysis)) {
        meleeParalysisCheck(ctx, v, monsterLevel);
    }
    if (hasBit(flags, kBitStone)) {
        stoneCheck(ctx, v, attacker.levelFound);  // engine form (R30) preferred
    }
    if (hasBit(flags, kBitDrain)) {
        const DrainOutcome out = drainCharacter(ctx, v, attacker.levelFound);
        for (i32 pick : out.picks) {
            applyDrainPick(ctx, v, pick);
        }
    }
    if (hasBit(flags, kBitAging)) {
        rollAgingDays(ctx, v, monsterLevel, attacker.floor);
    }
    // UNKNOWN: destruction (bit 12) needs an Inventory, not reachable here.
    // UNKNOWN: theft (bit 13) needs the thief coefficient, not supplied.
    // Step 5 (companion stoning of the monster) is not applied here; see companionBlowStones.
}

// R23: resistance reduces special-attack damage. No change if resistance <= 0.
// UNKNOWN: VbInt is taken as floor (Int); CInt would be the alternative.
i32 applySpecialResistance(i32 damage, i32 resistPercent) {
    if (resistPercent <= 0) return damage;
    const double d = static_cast<double>(damage);
    return static_cast<i32>(vbInt(d - d * ((resistPercent / 2.0) * 0.01)));
}

// R26: resistance index per special-attack kind.
// Breath: Fire (0); caller uses Cold (1) for cold breath (R24).
// UNKNOWN: acid and electrical indices are not in the SRD. The BreathType
// order (Fire, Cold, Acid, Electrical) is used as a guess: Acid=2, Electrical=3.
// FIXED: renamed from specialResistIndex (name clashes with header i32 declaration).
static i32 specialResistIndexOf(SpecialAttackKind kind) {
    switch (kind) {
        case SpecialAttackKind::Breath:     return kResFire;
        case SpecialAttackKind::AcidSpit:   return 2;  // UNKNOWN
        case SpecialAttackKind::Electrocute: return 3;  // UNKNOWN
    }
    return kResFire;
}

// R27: band of character level against monster level. Equal-to-band edges
// fall in Even (0.75M to 1.25M inclusive).
RatioBand ratioBand(i32 characterLevel, i32 monsterLevel) {
    const double c = static_cast<double>(characterLevel);
    const double m = static_cast<double>(monsterLevel);
    if (c > 1.25 * m) return RatioBand::High;
    if (c < 0.75 * m) return RatioBand::Low;
    return RatioBand::Even;
}

// R27 audit: breath threshold 80 / 50 / 20.
i32 breathThresholdPercent(i32 highestGuildLevel, i32 monsterLevel) {
    return kBreathThresholdPct[static_cast<u32>(ratioBand(highestGuildLevel, monsterLevel))];
}

// R27 audit: spit threshold 90 / 50 / 10 on the active level.
i32 spitThresholdPercent(i32 activeLevel, i32 monsterLevel) {
    return kSpitThresholdPct[static_cast<u32>(ratioBand(activeLevel, monsterLevel))];
}

// R28 breath damage (audit form).
// x = B(M*ln(M+1)); q = I((ln x + x/15)/ln 1.08 - L/20 + 1);
// dmg = B((I((U*q+1)/2) + q/4) * 0.7 / 2).
// UNKNOWN: B mode taken as round-half-even; U taken as nextDouble.
// A log of zero aborts the attack and returns 0.
i32 rollBreathDamage(Ctx& ctx, i32 monsterGuildLevel, i32 victimActiveLevel) {
    const double m = static_cast<double>(monsterGuildLevel);
    // RESOLVED-BY-REFERENCE: MonsterAttack.cs: "int l046C = VbInt(monLvl * Math.Log(monLvl + 1));".
    // This is a floor (VbInt), not round-half-even.
    const double x = static_cast<double>(vbInt(m * std::log(m + 1.0)));
    // RESOLVED-BY-REFERENCE: MonsterAttack.cs: "if (l046C <= 0) yield break;". The group turn ends there.
    // monsterGroupSpecialTurn takes that exit before calling this function; the return below is only a guard.
    if (x <= 0.0) return 0;
    const double q = static_cast<double>(vbInt(
        (std::log(x) + x / 15.0) / std::log(1.08)
        - static_cast<double>(victimActiveLevel) / 20.0 + 1.0));
    const double u = ctx.rng.nextDouble();
    const double inner = static_cast<double>(vbInt((u * q + 1.0) / 2.0));
    const double raw = (inner + q / 4.0) * 0.7 / 2.0;
    // RESOLVED-BY-REFERENCE: "int dmg = VbInt(((Floor(...) + range / 4.0) * 0.7) / 2);" - VbInt, not round-half-even.
    const i64 dmg = vbInt(raw);
    return dmg < 0 ? 0 : static_cast<i32>(dmg);
}

// R28 spit damage (audit form).
// x = M+1; q = I((ln x + x/15)/ln 1.08 - L/10 + 1);
// dmg = B((I((U*q+1)/2) + q/2) * 0.7).
// UNKNOWN: B mode taken as round-half-even; U taken as nextDouble.
i32 rollSpitDamage(Ctx& ctx, i32 monsterGuildLevel, i32 victimActiveLevel) {
    const double x = static_cast<double>(monsterGuildLevel) + 1.0;
    const double q = static_cast<double>(vbInt(
        (std::log(x) + x / 15.0) / std::log(1.08)
        - static_cast<double>(victimActiveLevel) / 10.0 + 1.0));
    const double u = ctx.rng.nextDouble();
    const double inner = static_cast<double>(vbInt((u * q + 1.0) / 2.0));
    const double raw = (inner + q / 2.0) * 0.7;
    const i64 dmg = roundHalfEven(raw);
    return dmg < 0 ? 0 : static_cast<i32>(dmg);
}

// R29: electrocution tier 90 / 50 / 10 by level.
// UNKNOWN: which level is used by the engine; the character level is used here
// (archive, c99).
i32 electrocutionChancePercent(i32 characterLevel, i32 monsterLevel) {
    return kElectrocutionThresholdPct[static_cast<u32>(ratioBand(characterLevel, monsterLevel))];
}

// R30 stoning chance: tCon*(ln(max(1,tCon))/ln 2.5) - LevelFound, capped at 95.
double stoneChance(const Character& target, i32 attackerLevel) {
    const i32 con = baseCon(target);
    const double tCon = static_cast<double>(con);
    const double lg = std::log(static_cast<double>(maxI(1, con))) / std::log(2.5);
    const double value = tCon * lg - static_cast<double>(attackerLevel);
    return minD(value, static_cast<double>(kStoneCap));
}

// R30 engine stoning check.
// UNKNOWN: the source comparator is garbled. Stoned when Int(Rnd*100)+1 > chance,
// matching the sibling checks.
bool stoneCheck(Ctx& ctx, Character& target, i32 attackerLevel) {
    const double chance = stoneChance(target, attackerLevel);
    if (static_cast<double>(d100(ctx)) > chance) {
        target.deathState = DeathState::Stoned;  // R3
        note(ctx, "status1a.stoned", "Stoned");
        return true;
    }
    return false;
}

// R30 companion stone value: CON*(ln(max(1,CON))/ln 2.5) - floor.
// UNKNOWN: general floor value; the test example uses 0.
double companionStoneValue(i32 companionCon, i32 stoneFloor) {
    const double con = static_cast<double>(companionCon);
    const double lg = std::log(maxD(1.0, con)) / std::log(2.5);
    return con * lg - static_cast<double>(stoneFloor);
}

// R22.5 / R30 test: stoned if Rnd*100 > stoneValue. The stone removes the
// target's hits.
bool companionBlowStones(Ctx& ctx, Creature& target, i32 companionCon, i32 stoneFloor) {
    const double value = companionStoneValue(companionCon, stoneFloor);
    if (ctx.rng.nextDouble() * 100.0 > value) {
        target.stoned = true;
        target.hits = 0;
        note(ctx, "status1a.monster_stoned", "Stoned one!");
        return true;
    }
    return false;
}

// R31 drain. Trials = Int(Rnd*VbInt(LevelFound/2+0.5)) + 1.
// Per trial: skip if roll <= Res(c,9) or roll <= CON*2.5 - LevelFound*2.
// A failed trial draws a Table E pick, Int(Rnd*6+1). Picks are returned for the
// caller to apply with applyDrainPick.
DrainOutcome drainCharacter(Ctx& ctx, Character& target, i32 attackerLevel) {
    DrainOutcome out;
    const i32 n = static_cast<i32>(vbInt(static_cast<double>(attackerLevel) / 2.0 + 0.5));
    out.trials = static_cast<i32>(ctx.rng.below(n)) + 1;
    const double conSave = static_cast<double>(baseCon(target)) * 2.5
                         - static_cast<double>(attackerLevel) * 2.0;
    for (i32 t = 0; t < out.trials; ++t) {
        const i32 roll = d100(ctx);
        if (roll <= resistAt(target, kResDrain) || static_cast<double>(roll) <= conSave) {
            continue;
        }
        out.picks.push_back(static_cast<i32>(ctx.rng.below(kDrainPickSides)) + 1);
    }
    return out;
}

// R31 Table E: picks 1..5 drain stat index = pick (-1); 6 drains MaxHits -2.
// Stat index 0 (Strength) is unreachable.
void applyDrainPick(Ctx& ctx, Character& c, i32 pick) {
    if (pick >= 1 && pick <= kDrainStatPickMax) {
        c.abilities.stats[static_cast<u32>(pick)] -= 1;
        note(ctx, "status1a.stat_drained", "Stat drained");
    } else if (pick == kDrainMaxHitsPick) {
        c.vitals.maxHits -= kDrainMaxHitsLoss;
        note(ctx, "status1a.maxhits_drained", "Maximum hits drained");
    }
}

// R32 aging save (audit): save at 2.5*CON - 2F.
double agingSaveChance(const Character& c, i32 monsterFloor) {
    return static_cast<double>(baseCon(c)) * 2.5
         - static_cast<double>(monsterFloor) * 2.0;
}

// R32 aging (audit). A save success returns 0. On failure, age
// min(1d1825, 365*clamp(B(M/50),1,5)) days.
// UNKNOWN: 1d1825 taken as uniform 1..1825; B mode taken as round-half-even.
// UNKNOWN: aged days are added to ageDays (daysAged not updated).
i32 rollAgingDays(Ctx& ctx, Character& c, i32 monsterGuildLevel, i32 monsterFloor) {
    const i32 roll = d100(ctx);
    if (static_cast<double>(roll) <= agingSaveChance(c, monsterFloor)) return 0;

    const i32 die = static_cast<i32>(ctx.rng.below(kAgingDaysCap)) + 1;
    const double unitRaw = static_cast<double>(monsterGuildLevel) / kAgingUnitDivisor;
    const i32 units = clampI(static_cast<i32>(roundHalfEven(unitRaw)),
                             kAgingUnitMin, kAgingUnitMax);
    const i32 cap = kAgingDaysPerUnit * units;
    const i32 days = die < cap ? die : cap;
    c.ageDays += days;
    note(ctx, "status1a.aged", "Aged");
    return days;
}

// R33 petrification (audit): first save at Stone resistance, then at
// min(95, CON*ln(max(1,CON))/ln 2.5 - F). Failure on both stones the character.
bool petrifyCheck(Ctx& ctx, Character& target, i32 monsterFloor) {
    const i32 r1 = d100(ctx);
    if (r1 <= resistAt(target, kResStone)) return false;  // first save held

    const i32 con = baseCon(target);
    const double lg = std::log(static_cast<double>(maxI(1, con))) / std::log(2.5);
    const double chance2 = minD(static_cast<double>(con) * lg - static_cast<double>(monsterFloor),
                                static_cast<double>(kParalysisCap));
    const i32 r2 = d100(ctx);
    if (static_cast<double>(r2) <= chance2) return false;  // second save held

    target.deathState = DeathState::Stoned;  // R3
    note(ctx, "status1a.petrified", "Petrified");
    return true;
}

// R35 destruction save (audit): clamp(45 - F + 4*DEX - 2*monDEX, 25, 90).
// UNKNOWN: the note "DEX term drops to DEX while paralysed" is not applied.
i32 destructionSaveChance(const Character& c, i32 monsterFloor, i32 monsterDex) {
    const i32 v = kDestructionSaveBase - monsterFloor
                + 4 * statAt(c, StatIndex::Dex) - 2 * monsterDex;
    return clampI(v, kDestructionSaveMin, kDestructionSaveMax);
}

// R35 destruction (audit). Samples a pack slot 1..39 (empty included), then
// saves. A failed save destroys the item in that slot.
// UNKNOWN: slot is sampled before the save; Item.destructible is not checked;
// id 0 is treated as an empty slot.
bool destructionCheck(Ctx& ctx, Character& c, Inventory& inv, i32 monsterFloor, i32 monsterDex) {
    const i32 slotCount = kDestructionSlotMax - kDestructionSlotMin + 1;
    const i32 slot = static_cast<i32>(ctx.rng.below(slotCount)) + kDestructionSlotMin;
    const i32 chance = destructionSaveChance(c, monsterFloor, monsterDex);
    if (d100(ctx) <= chance) return false;  // saved
    Item& it = inv.pack[static_cast<u32>(slot)];
    if (it.id == 0) return false;           // empty slot
    it = Item{};
    note(ctx, "status1a.item_destroyed", "Item destroyed");
    return true;
}

// R36 theft score: Int(Rnd*100) + I(U*(thiefLevel*thiefCoefficient) + 0.75*thiefLevel).
// UNKNOWN: "defend adds the integer half of that score" is taken as score/2 of the
// total score.
i32 theftScore(Ctx& ctx, i32 thiefLevel, i32 thiefCoefficient, bool defending) {
    const i32 base = static_cast<i32>(ctx.rng.below(kTheftScoreRange));
    const double u = ctx.rng.nextDouble();
    const double tl = static_cast<double>(thiefLevel);
    const i32 extra = static_cast<i32>(vbInt(u * (tl * static_cast<double>(thiefCoefficient))
                                             + 0.75 * tl));
    i32 score = base + extra;
    if (defending) score += score / 2;
    return score;
}

// R36: gold is attempted if score - 2M < 50.
bool theftGoldAttempted(i32 score, i32 monsterGuildLevel) {
    return score - kTheftGoldMonsterWeight * monsterGuildLevel < kTheftGoldThreshold;
}

// R36: item theft samples slots 2..40 (slot 40 is outside the pack).
i32 theftItemSlot(Ctx& ctx) {
    return static_cast<i32>(ctx.rng.below(kTheftSlotCount)) + kTheftSlotMin;
}

// ------------------------------------------------------------ archive (c99)

// R37 archive electrocution/acid damage. Lowest precedence.
// Minimum = (ln M + M/15)*4.55; Base = Minimum + Random[0..Minimum];
// -0.7 per 10 character levels; Final = Base*(1 - Res/2).
// UNKNOWN: archive form applied to electrocution; Random[a..b] taken as nextDouble;
// Resistance taken as a percent with the engine's (res/2)*0.01 convention.
// A log of zero (M <= 0) aborts and returns 0.
i32 archiveSpecialDamage(Ctx& ctx, i32 monsterGuildLevel, i32 characterLevel, i32 resistPercent) {
    if (monsterGuildLevel <= 0) return 0;  // UNKNOWN: log of zero aborts
    const double m = static_cast<double>(monsterGuildLevel);
    const double minimum = (std::log(m) + m / 15.0) * 4.55;
    const double base = minimum + ctx.rng.nextDouble() * minimum;
    const double levelAdjusted = base - 0.7 * static_cast<double>(characterLevel / 10);
    const double fin = levelAdjusted * (1.0 - (resistPercent / 2.0) * 0.01);
    const i64 dmg = vbInt(fin < 0.0 ? 0.0 : fin);
    return static_cast<i32>(dmg);
}

// R37 archive breath damage. LevelMod = M*ln M; Minimum = Int((ln LevelMod +
// LevelMod/15)*1.1375); Base = Minimum + Random[0..Minimum*2]; Final = Base*(1 - Res/2).
// UNKNOWN: the archive Minimum*2 is used (the R28 audit form is authoritative).
// A log of zero (M <= 1, LevelMod = 0) aborts and returns 0.
i32 archiveBreathDamage(Ctx& ctx, i32 monsterGuildLevel, i32 resistPercent) {
    const double m = static_cast<double>(monsterGuildLevel);
    const double levelMod = m * std::log(m);
    if (levelMod <= 0.0) return 0;  // UNKNOWN: log of zero aborts
    const double minimumD = vbInt((std::log(levelMod) + levelMod / 15.0) * 1.1375) >= 0
                          ? static_cast<double>(vbInt((std::log(levelMod) + levelMod / 15.0) * 1.1375))
                          : 0.0;
    const i32 minimum = static_cast<i32>(minimumD);
    const i32 base = minimum + static_cast<i32>(ctx.rng.below(minimum * 2 + 1));
    const double fin = static_cast<double>(base) * (1.0 - (resistPercent / 2.0) * 0.01);
    const i64 dmg = vbInt(fin < 0.0 ? 0.0 : fin);
    return static_cast<i32>(dmg);
}

// R37 archive stone chance: (100 + SpawnLevel) - CON*ln(CON)*1.09, minimum 5.
// UNKNOWN: ln(max(1,CON)) used so CON 0 does not produce ln(0).
i32 archiveStoneChance(i32 spawnLevel, i32 con) {
    const double c = static_cast<double>(con);
    const double v = (100.0 + static_cast<double>(spawnLevel))
                   - c * std::log(maxD(1.0, c)) * 1.09;
    return maxI(kArchiveChanceFloor, static_cast<i32>(vbInt(v)));
}

// R37 archive paralysis chance: 100 - (Resistance + CON), minimum 5.
i32 archiveParalysisChance(i32 resistPercent, i32 con) {
    return maxI(kArchiveChanceFloor, 100 - (resistPercent + con));
}

// R37 archive age chance: (100 + SpawnLevel*2) - CON*2.5.
i32 archiveAgeChance(i32 spawnLevel, i32 con) {
    const double v = (100.0 + static_cast<double>(spawnLevel) * 2.0)
                   - static_cast<double>(con) * 2.5;
    return static_cast<i32>(vbInt(v));
}

}  // namespace mordor

namespace mordor {

// Export wrapper for the file-local copy unparalyse in src/status1a.cpp.
void unparalyse_export_status1a(Ctx& ctx, Character& c) { return unparalyse__status1a(ctx, c); }

} // namespace mordor
