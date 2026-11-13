// combat1b.cpp: monster-round melee, special attacks, mishaps, companion pass hooks,
// victory sequence and archive formulas. Source: SRD c07p1 R32-R52.
#include <cmath>

#include "mordor/combat1b.h"
#include "mordor/companions1b.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"

namespace mordor {

namespace {

// R33 free-attack constants (not in header).
constexpr i32 kFreeAttackBase = 50;
constexpr i32 kFreeAttackPerDex = 5;
constexpr i32 kFreeAttackFloor = 5;

// R34 living-companion test. UNKNOWN: SRD does not define "living"; occupied, not dead,
// and in Normal death state is used.
bool isLivingCompanionSlot(const Companion& c) {
    return c.occupied && !c.isDead && c.deathState == CompanionDeathState::Normal;
}

}  // namespace

// R32. Every swing of every hand counts as one weapon blow.
void countWeaponBlow(Ctx& ctx, Character& ch) {
    (void)ctx;
    ch.weaponBlows += 1;
}

// R33. Free-attack chance in percent.
i32 freeAttackChance(AttackKind kind, i32 monDex, i32 charDex) {
    i32 chance = kFreeAttackBase + (monDex - charDex) * kFreeAttackPerDex;
    if (kind == AttackKind::ChangedLevel && chance > kFreeAttackChangedLevelCap) {
        chance = kFreeAttackChangedLevelCap;  // R33 stairs cap; Moved has no ceiling (quirk)
    }
    if (chance < kFreeAttackFloor) {
        chance = kFreeAttackFloor;
    }
    return chance;
}

// R33. attack = (Int(Rnd*100)+1) < chance.
bool rollFreeAttack(Ctx& ctx, AttackKind kind, i32 monDex, i32 charDex) {
    const i32 chance = freeAttackChance(kind, monDex, charDex);
    const i32 roll = static_cast<i32>(ctx.rng.below(kPercentDie)) + 1;
    return roll < chance;
}

// R34. Returns 0 for the character, or the 1-based companion slot picked.
// Returns 0 (character hit) if no living companion is found in kTargetPickTries tries.
i32 pickMonsterTarget(Ctx& ctx, const Party& party, const Arr<Companion, kCompanionSlots>& companions) {
    (void)party;  // R34 uses the companion array only.
    i32 alive = 0;
    for (i32 i = 0; i < kCompanionSlots; ++i) {
        if (isLivingCompanionSlot(companions[static_cast<u32>(i)])) {
            ++alive;
        }
    }
    i32 t = 0;
    // R34: if alive > 0 and (Rnd(100)+1) < 100.0/(alive+1): t = 0 (character).
    // The roll is drawn only when alive > 0 (short-circuit, as in the SRD).
    const bool characterTargeted =
        alive > 0 &&
        static_cast<f64>(ctx.rng.below(kPercentDie) + 1) < 100.0 / static_cast<f64>(alive + 1);
    if (characterTargeted) {
        t = 0;
    } else {
        // R34: else pick = Rnd(4)+1 until a living companion at that slot, 100 tries.
        // QUIRK (faithful): this loop also runs when alive == 0, drawing RNG each try.
        for (i32 tries = 0; tries < kTargetPickTries; ++tries) {
            const i32 pick = static_cast<i32>(ctx.rng.below(kCompanionSlots)) + 1;
            if (isLivingCompanionSlot(companions[static_cast<u32>(pick - 1)])) {
                t = pick;
                break;
            }
        }
    }
    return t;
}

// R35. Monster melee damage modifiers, in SRD order.
f64 meleeDamageModifier(Ctx& ctx, const MeleeModInput& in) {
    // CONFLICT: T10 table says floor 0.1 when caller passes <= 0; R35 text says 1. Using R35.
    // Reference (Combat.cs SwingOnce) passes weapon.DamageMod straight through and shows no floor.
    f64 damMod = in.damageMod <= 0.0 ? 1.0 : in.damageMod;

    // Target invisible and attacker lacks see-invisible (bit 0).
    if (in.targetInvisible && !in.attackerSeesInvisible) {
        damMod -= kInvisibleMalus;
    }
    // Character opening chest.
    if (in.targetOpeningChest) {
        damMod += kOpeningChestBonus;
    }
    // bit10 crit. CONFLICT: SRD text says Rnd(100)+1 <= chance; reference (Combat.cs SwingOnce)
    // says `_rng.Next(100) < chance`, i.e. Int(Rnd*100) against a Single, fractional part counts.
    // Using the reference form.
    if (in.critCapable) {
        const f64 chance = monsterCritChance(in.monsterLevel, in.levelFound, in.characterLevel, false);
        if (static_cast<f64>(ctx.rng.below(kPercentDie)) < chance) {
            damMod += kCritDamageBonus;
        }
    }
    // bit11 backstab: same test with backstab = true (reference form, as above).
    if (in.backstabCapable) {
        const f64 chance = monsterCritChance(in.monsterLevel, in.levelFound, in.characterLevel, true);
        if (static_cast<f64>(ctx.rng.below(kPercentDie)) < chance) {
            damMod += kBackstabDamageBonus;
        }
    }
    // Size: +sizeDiff/10 if target smaller than monster, -sizeDiff/10 if larger.
    damMod += sizeDamageAdjust(in.targetSize, in.monsterSize);
    return damMod;
}

// R35. Size adjustment added to damMod.
f64 sizeDamageAdjust(i32 targetSize, i32 monsterSize) {
    const i32 diff = targetSize > monsterSize ? targetSize - monsterSize : monsterSize - targetSize;
    const f64 step = static_cast<f64>(diff) / kSizeStepDivisor;
    if (targetSize < monsterSize) {
        return step;
    }
    if (targetSize > monsterSize) {
        return -step;
    }
    return 0.0;
}

// R36. ProtectionBonus = Protection > 0 ? VbInt(Log(P) * (Log(P)*3)) : 0.
i64 protectionBonus(i32 protection) {
    if (protection <= 0) {
        return 0;
    }
    const f64 lg = std::log(static_cast<f64>(protection));
    return vbInt(lg * (lg * kProtectionLogScale));
}

// R36. Protection bonus added to defence.
i32 defenceWithProtection(i32 defence, i32 protection) {
    return defence + static_cast<i32>(protectionBonus(protection));
}

// R35/R37. Monster melee blow. UNKNOWN: MonsterBlow body is not in the sources.
// Placeholder: archive monster form (R52) with the R35 damMod. The archive LN(DamMod)^2
// reduction (R35) is not applied. CONFLICT: monster reduction form (archive R52 vs engine).
i32 monsterBlow(Ctx& ctx, i32 monAtt, i32 targetDef, i32 monLvl, i32 monStr, f64 damMod) {
    return static_cast<i32>(archiveMonsterDamage(ctx, monStr, monLvl, damMod, targetDef, monAtt));
}

// R37. Melee resolves as modifiers (R35), then MonsterBlow.
i32 resolveMonsterMelee(Ctx& ctx, const MeleeModInput& in, i32 monAtt, i32 targetDef, i32 monStr) {
    const f64 damMod = meleeDamageModifier(ctx, in);
    return monsterBlow(ctx, monAtt, targetDef, in.monsterLevel, monStr, damMod);
}

// R38. A group may cast only if no spell has been used and the group's attack has not ended.
bool groupMayCastSpell(const GroupAttackFlags& flags) {
    return !flags.spellUsed && !flags.attackEnded;
}

// R38. Only the first monster spell attack per group is used; it ends the group's attack.
void recordGroupSpellAttack(Ctx& ctx, GroupAttackFlags& flags) {
    (void)ctx;
    flags.spellUsed = true;
    flags.attackEnded = true;
}

// R39. range = Int((Log(a) + a/15) / Log(1.08) - targetLevel/10 + 1), a = attackerGuildLevel+1.
// UNKNOWN: targetLevel/10 taken as integer division (header divisor is i32).
i32 specialAttackRange(i32 attackerGuildLevel, i32 targetLevel) {
    const f64 a = static_cast<f64>(attackerGuildLevel + 1);
    const f64 value = (std::log(a) + a / kSpecialRangeDivisor) / std::log(kSpecialRangeLogBase) -
                      static_cast<f64>(targetLevel / kSpecialRangeLevelDivisor) + 1.0;
    return static_cast<i32>(vbInt(value));
}

// R40. share = range/4 (breath), range/2 (acid spit, electrocute).
// UNKNOWN: integer division (header divisors are i32).
i32 specialAttackShare(i32 range, SpecialAttackKind kind) {
    if (kind == SpecialAttackKind::Breath) {
        return range / kBreathShareDivisor;
    }
    return range / kSpitShareDivisor;
}

// R40. dmg = VbInt((Int((Rnd*range + 1)/2) + share) * 0.7).
// CONFLICT: R40 wording says "rounded half to even"; the formula says VbInt (floor). Using VbInt.
i32 specialAttackRawDamage(Ctx& ctx, i32 range, SpecialAttackKind kind) {
    const f64 rnd = ctx.rng.nextDouble();
    const i64 half = vbInt((rnd * static_cast<f64>(range) + 1.0) / 2.0);
    const i32 share = specialAttackShare(range, kind);
    return static_cast<i32>(vbInt((static_cast<f64>(half) + static_cast<f64>(share)) * kSpecialDamageScale));
}

// R40 quirk. A monster's own breath uses sub0E85 (MONSMOD.BAS:2702-2704).
// UNKNOWN: exact expression is not in the chunk. Reading used: level*Log(level+1),
// minus targetLevel/20, halved. Not interchangeable with R39/R40.
i32 monsterBreathDamage(Ctx& ctx, i32 monLevel, i32 targetLevel) {
    (void)ctx;
    if (monLevel < 0) {
        return 0;
    }
    const f64 lvl = static_cast<f64>(monLevel);
    const f64 base = lvl * std::log(lvl + 1.0);
    const f64 dmg = (base - static_cast<f64>(targetLevel) / 20.0) / 2.0;
    if (dmg < 0.0) {
        return 0;
    }
    return static_cast<i32>(vbInt(dmg));
}

// R42. CONFLICT: engine comment says defence x1.5 when defending; audit says the bonus is
// overwritten and never applies. Unresolved in sources. Engine preferred: apply x1.5.
i32 defendingDefence(i32 defence, bool defending) {
    if (!defending) {
        return defence;
    }
    return static_cast<i32>(vbInt(static_cast<f64>(defence) * kDefendFactor));
}

// R43. One pass over the four slots for one owner. Shared one-cast-per-round gate.
// UNKNOWN: breath, spit, cast and strike bodies live in companions1b/companions2a and are
// not specified in this chunk, so only the pass structure and the gate are implemented.
void runOwnerCompanionPass(Ctx& ctx, Party& party, Arr<Companion, kCompanionSlots>& companions,
                           Encounter& enc, i32 ownerIndex) {
    (void)ctx;
    (void)party;
    (void)enc;
    GroupAttackFlags gate{};
    for (i32 slot = 0; slot < kCompanionSlots; ++slot) {
        const Companion& c = companions[static_cast<u32>(slot)];
        if (!isLivingCompanionSlot(c) || c.owner != ownerIndex) {
            continue;
        }
        // Breath or spit first (UNKNOWN action body).
        // Then cast or strike, subject to the shared gate.
        if (groupMayCastSpell(gate)) {
            // UNKNOWN: cast action body; gate is not consumed here because the cast
            // body is not defined in the sources.
        }
    }
}

// R44. clamp((999/max(1,M)) * (bodies/20) * factor, lo, hi).
// UNKNOWN: "bodies/20" taken as floating division (header divisor is i32).
f64 mishapChance(MishapSource source, i32 monsterLevel, i32 partyBodies) {
    MishapTuning tuning = kMishapSpell;
    if (source == MishapSource::Breath) {
        tuning = kMishapBreath;
    } else if (source == MishapSource::SpitOrMelee) {
        tuning = kMishapSpitMelee;
    }
    const i32 m = monsterLevel < 1 ? 1 : monsterLevel;
    const f64 perLevel = static_cast<f64>(kMishapNumerator) / static_cast<f64>(m);
    const f64 bodyShare = static_cast<f64>(partyBodies) / static_cast<f64>(kMishapBodyDivisor);
    const f64 raw = perLevel * bodyShare * tuning.factor;
    return clampv(raw, tuning.lo, tuning.hi);
}

// R45. Drawing the owner cancels the misdirection.
void cancelMisdirectionOnDraw(Ctx& ctx, Character& owner) {
    (void)ctx;
    owner.misdirected = false;
}

// R45. An owner can be freed from paralysis when companions finish the encounter.
// UNKNOWN: ParalysisPasses check is not defined in the sources; freed unconditionally
// once companions have finished.
void freeOwnerFromParalysis(Ctx& ctx, Character& owner, bool companionsFinishedEncounter) {
    (void)ctx;
    if (!companionsFinishedEncounter) {
        return;
    }
    if (owner.ailments.paralysed) {
        owner.ailments.paralysed = false;
        owner.ailments.paralyzedRounds = 0;
    }
}

// R46. Victory sequence, in SRD order.
void runVictorySequence(Ctx& ctx, Party& party, FloorState& floorState, i32 roomIndex, Encounter& enc,
                        i64 nowMs, i32 headerAreaCount) {
    // Step 1: store the fight's room. UNKNOWN: FloorState layout not in sources.
    // Step 2: mark the room cleared (only if Area > 0).
    markRoomCleared(ctx, floorState, roomIndex, nowMs, headerAreaCount);
    // Steps 3-4: award and pay fight gold.
    payFightGold(ctx, party);
    // Step 5: store the room again. UNKNOWN: see markRoomCleared.

    // Step 6: each party member records every monster that died.
    for (const Creature& m : enc.monsters) {
        if (m.alive) {  // UNKNOWN: "died" test; Creature.alive used.
            continue;
        }
        for (i32 i = 0; i < party.size; ++i) {
            noteMonsterKill(ctx, party.members[static_cast<u32>(i)], m);
        }
    }

    // Step 7: for each recordable kill, check the Hall of Records (DeadliestCreature).
    // UNKNOWN: which member is the killer; party.killer used when valid.
    for (const Creature& m : enc.monsters) {
        if (m.alive) {
            continue;
        }
        if (party.killer >= 0 && party.killer < party.size) {
            checkDeadliestCreature(ctx, party.members[static_cast<u32>(party.killer)], m.id);
        }
    }
}

// R46 step 2. UNKNOWN: FloorState and room layout are not in the sources, so the
// room-cleared write (only when Room.Area > 0) cannot be performed here.
void markRoomCleared(Ctx& ctx, FloorState& floorState, i32 roomIndex, i64 nowMs, i32 headerAreaCount) {
    (void)ctx;
    (void)floorState;
    (void)roomIndex;
    (void)nowMs;
    (void)headerAreaCount;
}

// R46 step 3. Gold split across the party. UNKNOWN: rounding; floor division used.
Currency splitFightGold(Currency fightGold, i32 partySize) {
    if (partySize <= 0) {
        return 0;
    }
    return fightGold / static_cast<Currency>(partySize);
}

// R46 steps 3-5. Pay each member an equal share. UNKNOWN: which members are paid
// (all used) and where the remainder goes (goldLeaderIndex used). Fight gold is emptied.
void payFightGold(Ctx& ctx, Party& party) {
    (void)ctx;
    if (party.size <= 0) {
        return;
    }
    const Currency share = splitFightGold(party.fightGold, party.size);
    const Currency remainder = party.fightGold - share * static_cast<Currency>(party.size);
    for (i32 i = 0; i < party.size; ++i) {
        party.members[static_cast<u32>(i)].wallet.goldOnHand += share;
    }
    if (party.goldLeaderIndex >= 0 && party.goldLeaderIndex < party.size) {
        party.members[static_cast<u32>(party.goldLeaderIndex)].wallet.goldOnHand += remainder;
    }
    party.fightGold = 0;
}

// R46 step 6. NoteMonster(killed: true). UNKNOWN: exact record semantics; the kill is
// counted and the monster id appended to monsterKills.
void noteMonsterKill(Ctx& ctx, Character& member, const Creature& monster) {
    (void)ctx;
    member.kills += 1;
    member.monsterKills.push_back(monster.id);
}

// R46 step 7. UNKNOWN: hall.Check body and MonsterPower definition are not in the sources.
// No record is written.
void checkDeadliestCreature(Ctx& ctx, Character& killer, i32 monsterId) {
    (void)ctx;
    (void)killer;
    (void)monsterId;
}

// R47. Experience is announced only when it takes a character past a level.
void announceExperienceIfLevelUp(Ctx& ctx, const Character& ch, i32 levelBefore) {
    if (ch.level > levelBefore) {
        ctx.say("combat1b.level_up", "A character has gained a level.", {levelBefore, ch.level}, {ch.name});
    }
}

// R48. A chest belongs to the area it was made in and is available only there.
// No Encounter source, or Room.Area <= 0, means it belongs to wherever you stand.
bool chestAvailableHere(bool hasEncounterSource, i32 roomArea, i32 standingArea) {
    if (!hasEncounterSource || roomArea <= 0) {
        return true;
    }
    return roomArea == standingArea;
}

// R50 (archive, unverified). threshold = ItemLevel*2 + 1.
// CONFLICT: not reconciled with bit 4 (R13); engine preferred for the port is
// Floor >= floor(LevelFound/2). This archive form is kept for reference.
i32 archiveWeaponResistThreshold(i32 itemLevel) {
    return itemLevel * 2 + 1;
}

// R51 (archive). Hit chance in percent, clamped to [kArchiveHitMin, kArchiveHitMax].
// CONFLICT: engine clamp is 2-98; archive clamp 1-97 is used here.
f64 archiveHitChancePercent(i32 atk, i32 def) {
    if (atk == 0 && def == 0) {
        return 50.0;
    }
    f64 p = 0.5;
    if (atk > def) {
        p = 0.5 + 0.5 * static_cast<f64>(atk - def) / static_cast<f64>(atk);
    } else if (atk < def) {
        p = 0.5 - 0.5 * static_cast<f64>(def - atk) / static_cast<f64>(def);
    }
    // UNKNOWN: Atk == Def with Def != 0 is not stated; parity 50% used (p = 0.5).
    return clampv(p * 100.0, static_cast<f64>(kArchiveHitMin), static_cast<f64>(kArchiveHitMax));
}

// R52 (archive). Monster base melee damage with defence reduction.
// UNKNOWN: LN is undefined for MonStrength <= 0; strength clamped to 1.
// UNKNOWN: reduction lower bound when CharDef < MonAtk; span clamped to 0.
f64 archiveMonsterDamage(Ctx& ctx, i32 monStrength, i32 monGuildLevel, f64 damMod,
                         i32 charDef, i32 monAtk) {
    const f64 str = static_cast<f64>(monStrength < 1 ? 1 : monStrength);
    const f64 ln = std::log(str);
    const i64 bsv = vbInt(str - ln * ((ln - 2.4) * 2.25)) + 1;
    const f64 bsvD = static_cast<f64>(bsv);

    const f64 rnd = ctx.rng.nextDouble();
    const f64 strMod1 =
        static_cast<f64>(vbInt(rnd * ((bsvD + 10.0) / 2.0) + (bsvD + 10.0) / 4.0)) / 10.0;
    const f64 strMod2 = bsvD / 30.0;
    const f64 lvlMod = (std::log(static_cast<f64>(monGuildLevel + 5)) + 1.0) / std::log(1.2);
    const f64 base = lvlMod * strMod1 * strMod2 * damMod;

    i32 span = static_cast<i32>(vbInt(static_cast<f64>(charDef - monAtk) / 8.0));
    if (span < 0) {
        span = 0;
    }
    const i32 reduce = static_cast<i32>(ctx.rng.below(span + 1));  // Random[0 to span]

    const i64 finalDmg = vbInt(base) - static_cast<i64>(reduce);
    return finalDmg < 0 ? 0.0 : static_cast<f64>(finalDmg);
}

// Archive player form (c97): Final = Base + Rnd[1 to Atk/8]. Inferred.
i32 archivePlayerDamage(Ctx& ctx, f64 baseDamage, i32 atk) {
    const i32 top = static_cast<i32>(vbInt(static_cast<f64>(atk) / 8.0));
    i32 extra = 0;
    if (top >= 1) {
        extra = static_cast<i32>(ctx.rng.below(top)) + 1;
    }
    return static_cast<i32>(vbInt(baseDamage)) + extra;
}

}  // namespace mordor
