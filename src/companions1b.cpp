#include "mordor/companions1b.h"

#include <cmath>

namespace mordor {
namespace {
// UNKNOWN: Att of a companion caster is not identified in R27.3; Fight (Att 2) is used.
constexpr i32 kUnknownCasterAtt = 2;

i32 absI(i32 v) {
    return v < 0 ? -v : v;
}
}  // namespace

// ---------------------------------------------------------------- R27 spell or blow

// R27.1: a companion casts only if no companion has cast for this owner and anti-magic is absent.
bool companionMayCast(bool ownerCastThisRound, bool antiMagicPresent) {
    return !ownerCastThisRound && !antiMagicPresent;
}

// R27.2: the cast uses a charge from the owner's item in the owner's pack slot.
bool companionTakeCastCharge(Ctx& ctx, Inventory& ownerInv, i32 packSlot) {
    if (packSlot < 0 || packSlot >= kPackSlots) {
        // UNKNOWN: invalid pack slot is treated as an out-of-charges refusal.
        ctx.say("companion.outOfCharges", "Out of charges.", {}, {});
        return false;
    }
    Item& it = ownerInv.pack[static_cast<u32>(packSlot)];
    if (it.charges <= 0) {
        // R27.2 refusal line. UNKNOWN: the "sub102F owner, -1" call is not modelled here.
        ctx.say("companion.outOfCharges", "Out of charges.", {}, {});
        return false;
    }
    it.charges -= 1;  // UNKNOWN: one charge per cast assumed.
    return true;
}

// R27.3: owner order goes to Defend (Att 0 or 1) or Fight (Att 2).
CombatOrderKind orderAfterCompanionCast(i32 att) {
    return (att == 0 || att == 1) ? CombatOrderKind::Defend : CombatOrderKind::Fight;
}

// R27.5: a non-Mage companion caster gives up half the time (fn0FF5).
bool companionCastFizzles(Ctx& ctx, bool casterIsMage) {
    if (casterIsMage) {
        return false;
    }
    return ctx.rng.percent(kCasterFizzlePercent);
}

// R27.5: companion caster failure 50%.
bool companionCastFails(Ctx& ctx) {
    return ctx.rng.percent(kCasterFailPercent);
}

// R27.6: companions cast at guild level / 6.
i32 companionCastLevel(i32 guildLevel) {
    return static_cast<i32>(vbInt(static_cast<f64>(guildLevel) / static_cast<f64>(kCompanionCastLevelDivisor)));
}

// R27.6: heals (M5B1D = 0) reach nobody; a heal passed a slot is thrown at monsters as damage.
HealRoute companionHealRoute(bool passedToSlot, i32 healTargetFlag) {
    if (healTargetFlag == 0) {
        return HealRoute::Nobody;
    }
    if (passedToSlot) {
        return HealRoute::MonstersAsDamage;
    }
    return HealRoute::Party;
}

// R27.7: magic immunity is tested on the body itself (2556).
// UNKNOWN: the line-2556 test is not in this part; the Creature flag is used.
bool companionMagicImmune(const Creature& body) {
    return body.magicImmune;
}

// R27.9: spell odds x 1.5, clamped 1..40.
f64 companionSpellMishapChance(f64 spellOdds) {
    return clampv(spellOdds * kSpellMishapFactor, kSpellMishapFloor, kSpellMishapCeiling);
}

// R27.9: l01D0 = Int(Rnd * 100) is drawn and unused; then the mishap test.
// UNKNOWN: mishap comparison form is not in the SRD; rnd(100)+1 <= odds (as R28) is used.
bool companionSpellMishapRoll(Ctx& ctx, f64 spellOdds) {
    (void)ctx.rng.below(100);
    const f64 chance = companionSpellMishapChance(spellOdds);
    return static_cast<f64>(ctx.rng.below(100) + 1) <= chance;
}

// ---------------------------------------------------------------- R28 blows and flags

bool specialFlagBit(u32 flags, i32 bit) {
    if (bit < 0 || bit > 31) {
        return false;
    }
    return ((flags >> static_cast<u32>(bit)) & 1u) != 0u;
}

// UNKNOWN: MonsterCritChance formula is not in this SRD part (R28 uses it by name).
// Returns 0 so that no crit lands until the formula is supplied.
f64 monsterCritChance(i32 guildLevel, i32 levelFound, i32 targetLevel, bool backstab) {
    (void)guildLevel;
    (void)levelFound;
    (void)targetLevel;
    (void)backstab;
    return 0.0;
}

// R28 damMod steps. VB "And" is not short-circuit, so both rnd(100) rolls are drawn
// whether or not the flag bit is set. UNKNOWN: confirm this reading.
i32 blowDamageMod(Ctx& ctx, const BlowParams& p, bool targetUnseen, i32 targetLevel) {
    i32 damMod = p.damageMod;
    const bool critBit = specialFlagBit(p.specialFlags, kBlowFlagCrit);
    const bool critHit = static_cast<f64>(ctx.rng.below(100) + 1) <=
                         monsterCritChance(p.guildLevel, p.levelFound, targetLevel, false);
    if (critBit && critHit) {
        damMod += kBlowCritBonus;  // R28 +4
    }
    const bool backBit = specialFlagBit(p.specialFlags, kBlowFlagBackstab);
    const bool backHit = static_cast<f64>(ctx.rng.below(100) + 1) <=
                         monsterCritChance(p.guildLevel, p.levelFound, targetLevel, true);
    if (backBit && backHit) {
        damMod += kBlowBackstabBonus;  // R28 +1
    }
    // R28: Has(def,0) is a constant 0 here, so the invisible exemption is never granted.
    const bool exemptByProperty = false;
    if (targetUnseen && !exemptByProperty) {
        damMod -= damMod / 2;  // integer division, truncates toward zero
    }
    return damMod;
}

// UNKNOWN: Rules.Fn0DEE body is not in this SRD part. Placeholder: damage equals the
// modified damage modifier, floored at 0 (no minimum of 1, R28).
i32 blowDamageRoll(Ctx& ctx, const BlowParams& p, i32 targetDef, i32 damMod) {
    (void)ctx;
    (void)p;
    (void)targetDef;
    return damMod < 0 ? 0 : damMod;
}

// R28: damMod, then damage (Fn0DEE).
i32 blowDamage(Ctx& ctx, const BlowParams& p, i32 targetDef, i32 targetLevel, bool targetUnseen) {
    const i32 damMod = blowDamageMod(ctx, p, targetUnseen, targetLevel);
    return blowDamageRoll(ctx, p, targetDef, damMod);
}

// R29: after = damage - damage * (resist / 2) / 100.
// UNKNOWN: odd resist uses integer halving (truncation); the SRD does not say.
i32 afterResistance(i32 damage, i32 resistPercent) {
    const i64 half = resistPercent / kResistHalfDivisor;
    const i64 d = damage;
    return static_cast<i32>(d - (d * half) / 100);
}

// R30: chance = con * (Log(Max(1,con)) / Log(2.5)) - LevelFound, capped at 95.
f64 stoneChance(i32 con, i32 levelFound) {
    const i32 safeCon = con < 1 ? 1 : con;
    f64 chance = static_cast<f64>(con) *
                     (std::log(static_cast<f64>(safeCon)) / std::log(kStoneLogBase)) -
                 static_cast<f64>(levelFound);
    if (chance > 95.0) {
        chance = 95.0;  // R30 cap
    }
    return chance;
}

// R30: res = Res(target,7); if rnd(100)+1 > res, then stoned when rnd()*100 > chance.
// (Literal SRD test: the higher the chance, the less often the stone lands; this reproduces
// the documented 74.87% for CON 10 against LevelFound 0.)
bool stoneLands(Ctx& ctx, i32 stoneRes, i32 stonerCon, i32 levelFound) {
    if (ctx.rng.below(100) + 1 > stoneRes) {
        const f64 chance = stoneChance(stonerCon, levelFound);
        if (ctx.rng.nextDouble() * 100.0 > chance) {
            return true;
        }
    }
    return false;
}

// Follow-up tests Foes(...).Hits > 0 from the start index (line 1215, R30 note).
i32 nextLiveFoe(const Vec<Creature>& foes, i32 startIndex) {
    const i32 n = static_cast<i32>(foes.size());
    const i32 start = startIndex < 0 ? 0 : startIndex;
    for (i32 i = start; i < n; ++i) {
        if (foes[static_cast<u32>(i)].hits > 0) {
            return i;
        }
    }
    return -1;
}

// UNKNOWN: Raw(Electrocute, ...) formula is not in this SRD part. Placeholder returns 0.
i32 rawElectrocuteDamage(i32 guildLevel, i32 targetLevel) {
    (void)guildLevel;
    (void)targetLevel;
    return 0;
}

// R31: zap = AfterResistance(Raw(Electrocute, GuildLevel, targetLevel), Res(target, Electrical)).
i32 electrocutionDamage(i32 guildLevel, i32 targetLevel, i32 electricalResistPercent) {
    return afterResistance(rawElectrocuteDamage(guildLevel, targetLevel), electricalResistPercent);
}

// R28 final sentence: stone (bit 8) then electrocute (bit 6) apply on a landed blow.
// Returns the electrocution damage dealt (0 if none).
// UNKNOWN: target death bookkeeping after stoning is handled elsewhere; only the flag is set.
i32 applyLandedBlowEffects(Ctx& ctx, const BlowParams& p, Creature& target, i32 targetLevel,
                           i32 stoneRes, i32 electricalResistPercent) {
    if (specialFlagBit(p.specialFlags, kBlowFlagStone)) {
        if (stoneLands(ctx, stoneRes, p.stonerCon, p.levelFound)) {
            target.stoned = true;
        }
    }
    i32 dealt = 0;
    if (specialFlagBit(p.specialFlags, kBlowFlagElectrocute)) {
        dealt = electrocutionDamage(p.guildLevel, targetLevel, electricalResistPercent);
        target.hits -= dealt;
    }
    return dealt;
}

// ---------------------------------------------------------------- R32 mishap sounds and captions

// R32.1-2: death cry DI-UNDEA plays once, on its own line, before the mishap line.
void mishapDeathCry(Ctx& ctx, bool suppressed) {
    if (suppressed) {
        return;
    }
    ctx.say("mishap.deathCry", "DI-UNDEA", {}, {});
}

// R32.3: melee companion hitting a party member (no kill) plays DRAGON; owner hears it.
void mishapAttackCry(Ctx& ctx) {
    ctx.say("mishap.attackCry", "DRAGON", {}, {});
}

// R32.4-6. HitFor only when the on-screen character is hit; ElectrocutedFor always.
// UNKNOWN: caption wording beyond the two quoted prefixes is assumed.
void mishapCaption(Ctx& ctx, MishapCaption kind, bool watchedCharacterHit, Str victimName, bool andCompanions) {
    const bool electro = kind == MishapCaption::ElectrocutedFor;
    if (!electro && !watchedCharacterHit) {
        return;
    }
    Str text(electro ? "You were accidently electrocuted for" : "You were accidently hit for");
    text += " ";
    text += victimName;
    if (andCompanions) {
        text += " and his companions";  // R32.6
    }
    ctx.say(electro ? "mishap.electrocuted" : "mishap.hit", text, {}, {victimName});
}

// R32.6: suffix is reset on each pass, so it names only the last slot visited.
bool mishapSuffixApplies(i32 slotVisited, i32 lastSlotVisited) {
    return slotVisited == lastSlotVisited;
}

// R33: companion kills are reported to the mods as a Companion-kind kill for the owner.
// CONFLICT: docs (c36) say companion kills count toward owner Kills; engine (c5) preferred,
// so owner.kills is not changed here (const parameter).
void reportCompanionKill(Ctx& ctx, const Character& owner, i32 monsterId) {
    ctx.say("companion.kill", "Companion kill reported.", {static_cast<i64>(monsterId)}, {owner.name});
}

// ---------------------------------------------------------------- R34 drift

// R34 drift content (Binding < 0): max(0.5, (MaxHits - Hits) / MaxHits * 15); 0 if MaxHits <= 0.
f64 companionDriftContent(i32 hits, i32 maxHits) {
    // R34 drift content (Binding < 0): max(0.5, (MaxHits - Hits) / MaxHits * 15); 0 if MaxHits <= 0.
    // The floor and scale are written out from the SRD. Check that kDriftContentFloor
    // is 0.5 and kDriftContentScale is 15.0 wherever they are defined.
    // The floor is applied with an explicit comparison, not maxv, so fractional results are kept.
    constexpr f64 kContentFloor = 0.5;
    constexpr f64 kContentScale = 15.0;
    if (maxHits <= 0) {
        return 0.0;
    }
    const f64 raw = static_cast<f64>(maxHits - hits) / static_cast<f64>(maxHits) * kContentScale;
    return (raw > kContentFloor) ? raw : kContentFloor;
}

// R34 drift bound (Binding > 0):
// max(1, GuildLevel / ln(3) * 2 - TopGuildLevel / ln(12) * |Binding / 2|).
// ln is natural log (R60, not confirmed by docs).
f64 companionDriftBound(i32 guildLevel, i32 topGuildLevel, i32 bindingLevel) {
    // R34 drift bound (Binding > 0): max(1, GuildLevel / ln(3) * 2 - TopGuildLevel / ln(12) * |Binding / 2|)
    // The floor and formula are written out from the SRD so they do not depend on the
    // kDriftBoundFloor value. Check that kDriftBoundFloor is 1.0 wherever it is defined.
    // The floor is applied with an explicit comparison, not maxv, so fractional results are kept.
    constexpr f64 kBoundFloor = 1.0;
    const f64 a = static_cast<f64>(guildLevel) / std::log(3.0) * 2.0;
    const f64 b = static_cast<f64>(topGuildLevel) / std::log(12.0) *
                  std::fabs(static_cast<f64>(bindingLevel) / 2.0);
    const f64 raw = a - b;
    return (raw > kBoundFloor) ? raw : kBoundFloor;
}

// R34.1: CompanionMood.Round. ForLife is never touched.
// Drift: if rng()*200 < Drift then Binding moves one notch toward 0.
// CONFLICT: port runs once per combat round (c8, c43); docs (c28) say once a second.
// Engine (per round) used. UNKNOWN: a zero-binding companion still draws the roll.
void companionMoodRound(Ctx& ctx, Companion& c, i32 topGuildLevel) {
    if (c.forLife) {
        return;
    }
    f64 drift = 0.0;
    if (c.bindingLevel < 0) {
        drift = companionDriftContent(c.hits, c.maxHits);
    } else if (c.bindingLevel > 0) {
        drift = companionDriftBound(c.guildLevel, topGuildLevel, c.bindingLevel);
    }
    if (ctx.rng.nextDouble() * kDriftRollRange < drift) {
        c.bindingLevel += (c.bindingLevel > 0 ? -1 : 1);
    }
}

// R35 bound = round_half_even(|bindingLevel| * 4.5).
i32 boundFromBinding(i32 bindingLevel) {
    const f64 mag = static_cast<f64>(absI(bindingLevel)) * kBoundScale;
    return static_cast<i32>(roundHalfEven(mag));
}

// R35 chance to leave, clamped 0..90.
// CONFLICT: the pseudo-code sets chance = 0 for Binding 100 and then overwrites it with the
// general formula. Early return used here (matches the documented "never" for Binding 100;
// ForLife companions are also skipped in R34).
f64 companionChanceToLeave(const Companion& c, i32 topGuildLevel, i32 charisma) {
    if (c.bindingLevel == kBindingMax) {
        return 0.0;
    }
    const i32 bound = boundFromBinding(c.bindingLevel);
    const f64 mon = std::log(static_cast<f64>(c.guildLevel) + 1.0) * kMonsterLeaveScale;
    f64 x = mon * kMonsterLeaveWeight -
            (static_cast<f64>(bound) * kBoundWeight +
             std::log(static_cast<f64>(topGuildLevel) / kTopGuildLnDivisor + 1.0) * kTopGuildLnWeight);
    x = x - static_cast<f64>(charisma) / kChaDivisor;
    x = x / kLeaveNormDivisor * kLeaveNormScale;
    // UNKNOWN: maxHits == 0 would divide by zero; the hits term is then omitted.
    if (c.maxHits > 0) {
        x = x + static_cast<f64>(c.maxHits - c.hits) / static_cast<f64>(c.maxHits) * kHitsLeaveScale;
    }
    x = x / kLeaveFinalDivisor;
    return clampv(x, 0.0, static_cast<f64>(kChanceToLeaveMax));
}

// R35 describe table: 0 Completely; <=3 Excellently; <=10 Fairly; <=15 Marginally; <=20 Weakly; else Very Weakly.
Str companionLeaveDescribe(f64 chance) {
    if (chance <= 0.0) {
        return Str("Completely");
    }
    if (chance <= kLeaveDescribeLimit[0]) {
        return Str("Excellently");
    }
    if (chance <= kLeaveDescribeLimit[1]) {
        return Str("Fairly");
    }
    if (chance <= kLeaveDescribeLimit[2]) {
        return Str("Marginally");
    }
    if (chance <= kLeaveDescribeLimit[3]) {
        return Str("Weakly");
    }
    return Str("Very Weakly");
}

// R34 leave test: leaving if rng()*200 < ChanceToLeave, or Binding == 0. Roll always drawn.
bool companionLeaveRoll(Ctx& ctx, const Companion& c, f64 chance) {
    const bool roll = ctx.rng.nextDouble() * kLeaveRollRange < chance;
    return roll || c.bindingLevel == 0;
}

// R36 turn check. Both rnd1 and rnd2 are always drawn.
// Rules attached to R36 applied: Good creature never turns unless forced (rule 1);
// Binding <= 0 never turns unless forced (rule 2); a turn needs a free group slot (rule 3).
// CONFLICT: the gap rule |creature - owner| > 1 would let a Good creature turn under an Evil
// owner; rule 1 ("no chance of turning") is preferred here.
// UNKNOWN: refusal is checked only when the companion would turn.
// UNKNOWN: "fight alive" is taken as a non-empty foe list.
// CONFLICT: the 75% Evil roll is applied as written; with gap > 1 the companion already turns.
TurnOutcome companionTurnCheck(Ctx& ctx, const Companion& c, Align ownerAlign, CharAlign creatureAlign,
                               bool forced, const Vec<Creature>& foes, bool groupSlotFree) {
    const f64 rnd1 = ctx.rng.nextDouble();
    const f64 rnd2 = ctx.rng.nextDouble();
    const i32 creatureA = static_cast<i32>(creatureAlign);
    const i32 ownerA = static_cast<i32>(ownerAlign);

    bool turns = absI(creatureA - ownerA) > 1;
    if (creatureAlign == CharAlign::Evil && rnd1 * 100.0 < kTurnEvilPercent) {
        turns = true;
    }
    if (creatureAlign == CharAlign::Neutral && rnd2 * 100.0 < kTurnNeutralPercent) {
        turns = true;
    }
    if (forced) {
        turns = true;
    }
    if (!forced) {
        if (creatureAlign == CharAlign::Good) {
            turns = false;  // R36 rule 1
        }
        if (c.bindingLevel <= 0) {
            turns = false;  // R36 rule 2
        }
    }
    if (turns && !foes.empty() && absI(static_cast<i32>(foes[0].align) - creatureA) > 1) {
        ctx.say("companion.refuses", "Your companion refuses to join the current monsters!", {}, {});
        return TurnOutcome::RefusedByFoe;
    }
    if (turns && !groupSlotFree) {
        return TurnOutcome::NoGroupSlot;  // R36 rule 3
    }
    return turns ? TurnOutcome::Joins : TurnOutcome::Holds;
}

}  // namespace mordor
