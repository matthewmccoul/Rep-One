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

// R35/R37. Monster melee blow. UNKNOWN: MonsterBlow body is not in the sources.
// Placeholder: archive monster form (R52) with the R35 damMod. The archive LN(DamMod)^2
// reduction (R35) is not applied. CONFLICT: monster reduction form (archive R52 vs engine).
i32 monsterBlow(Ctx& ctx, i32 monAtt, i32 targetDef, i32 monLvl, i32 monStr, f64 damMod) {
    return static_cast<i32>(archiveMonsterDamage(ctx, monStr, monLvl, damMod, targetDef, monAtt));
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

}  // namespace mordor
