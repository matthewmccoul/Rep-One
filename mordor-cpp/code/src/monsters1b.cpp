// monsters1b.cpp
// Monster attack resolution (R37-R38), breath/acid/electrocution formulas (R39),
// step-up (R40), fear trap (R41), thief retarget (R40), dice and bit helpers (W15-W17).
// Functions declared in monsters1b.h are defined here or in monsters1b_2.cpp, each once.
#include "mordor/monsters1b.h"

#include <cmath>

#include "mordor/clock.h"
#include "mordor/math_util.h"

namespace mordor {

namespace {

constexpr i32 kFoeGroupSlots = 4;  // SquareState::foes is Arr<FoeGroup, 4>

// Party member at victimIndex, or nullptr when the index is not an occupied slot.
Character* victimAt(Party& party, i32 victimIndex) {
    if (victimIndex < 0 || victimIndex >= party.size || victimIndex >= kPartyMaxSize) {
        return nullptr;
    }
    return &party.members[static_cast<u32>(victimIndex)];
}

// Owner index for a companion; falls back to member 0 when the stored owner is out of range.
i32 ownerIndexOf(const Party& party, i32 owner) {
    if (owner < 0 || owner >= party.size || owner >= kPartyMaxSize) {
        return 0;
    }
    return owner;
}

// Applies hit damage. UNKNOWN: death state after hits reach zero is set by the death module.
void takeHits(Character& victim, i32 damage) {
    victim.vitals.curHits -= damage;
    if (victim.vitals.curHits < 0) {
        victim.vitals.curHits = 0;
    }
}

// R39 first block (breath):
//   Base = Int(((Int(((Rnd * M) + 1) / 2) + (M / 4)) * 0.7) / 2)
i32 breathBlockDamage(Ctx& ctx, i32 modifier) {
    const f64 m = static_cast<f64>(modifier);
    const f64 rnd = ctx.rng.nextDouble();
    const f64 inner = static_cast<f64>(vbInt((rnd * m + 1.0) / 2.0));
    return static_cast<i32>(vbInt(((inner + m / 4.0) * 0.7) / 2.0));
}

// R39 second block (acid and electrocution):
//   Base = Int((Int(((Rnd * M) + 1) / 2) + (M / 2)) * 0.7)
i32 secondBlockDamage(Ctx& ctx, i32 modifier) {
    const f64 m = static_cast<f64>(modifier);
    const f64 rnd = ctx.rng.nextDouble();
    const f64 inner = static_cast<f64>(vbInt((rnd * m + 1.0) / 2.0));
    return static_cast<i32>(vbInt((inner + m / 2.0) * 0.7));
}

}  // namespace

// R37. A fatal mishap is announced only by the death, so it is not announced separately.
bool mishapAnnouncedSeparately(bool fatal) {
    return !fatal;
}

// R38 pre-attack 1) Breath. Damage from R39 first block.
// UNKNOWN: the SRD does not map BreathType to the special attacks. Here Fire and Cold are
// breath, Acid is acid spit (monsterAcid), Electrical is electrocution (electrocute).
void monsterBreath(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex) {
    if (!attacker.hasSpecialAttack) {
        return;
    }
    if (attacker.breathType != BreathType::Fire && attacker.breathType != BreathType::Cold) {
        return;
    }
    Character* victim = victimAt(party, victimIndex);
    if (victim == nullptr) {
        return;
    }
    const i32 damage = breathBaseDamage(ctx, attacker.guildLevel, victim->activeLevel());
    takeHits(*victim, damage);
    ctx.say("monster.breath", "The breath hits for damage.", {damage}, {});
}

// R38 pre-attack 2) Acid. Damage from R39 second block (acid formula).
void monsterAcid(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex) {
    if (!attacker.hasSpecialAttack) {
        return;
    }
    if (attacker.breathType != BreathType::Acid) {
        return;
    }
    Character* victim = victimAt(party, victimIndex);
    if (victim == nullptr) {
        return;
    }
    const i32 damage = acidBaseDamage(ctx, attacker.guildLevel, victim->activeLevel());
    takeHits(*victim, damage);
    ctx.say("monster.acid", "The acid hits for damage.", {damage}, {});
}

// R38 pre-attack: 1) Breath, 2) Acid.
void monsterPreAttack(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex) {
    monsterBreath(ctx, party, attacker, victimIndex);
    monsterAcid(ctx, party, attacker, victimIndex);
}

// R38 attack 1b) Melee, up to kMeleeAttemptsMax (5) attempts. Returns attempts made.
// UNKNOWN: hit and damage rules for a melee attempt are not in this part; the combat
// modules own them. This counts attempts and stops when the victim is down.
i32 monsterMeleeAttempts(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex) {
    (void)ctx;
    (void)attacker;
    i32 attempts = 0;
    for (i32 a = 0; a < kMeleeAttemptsMax; ++a) {
        const Character* victim = victimAt(party, victimIndex);
        if (victim == nullptr || victim->vitals.curHits <= 0) {
            break;
        }
        attempts += 1;
    }
    return attempts;
}

// R38 post-attack 1) Electrocute. Damage from R39 second block (electrocution formula).
// UNKNOWN: gate is BreathType::Electrical; the SRD does not state the trigger.
void electrocute(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex) {
    if (attacker.breathType != BreathType::Electrical) {
        return;
    }
    Character* victim = victimAt(party, victimIndex);
    if (victim == nullptr) {
        return;
    }
    const i32 damage = electrocutionBaseDamage(ctx, attacker.guildLevel, victim->activeLevel());
    takeHits(*victim, damage);
    ctx.say("monster.electrocute", "The electrical discharge hits for damage.", {damage}, {});
}

// R38 post-attack 7) Destroy Item.
// UNKNOWN: item destruction rules are not in this part; no effect is applied here.
void monsterDestroyItem(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex) {
    (void)ctx;
    (void)party;
    (void)attacker;
    (void)victimIndex;
}

// R38 post-attack 8) Steal.
// UNKNOWN: theft rules are not in this part; no effect is applied here.
void monsterSteal(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex) {
    (void)ctx;
    (void)party;
    (void)attacker;
    (void)victimIndex;
}

// R38 post-attack order: 1) Electrocute, 2) Age, 3) Drain, 4) Stone, 5) Poison,
// 6) Disease, 7) Destroy Item, 8) Steal.
// UNKNOWN: steps 2-6 have no rules in this part and are not called from here.
void monsterPostAttack(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex) {
    electrocute(ctx, party, attacker, victimIndex);
    monsterDestroyItem(ctx, party, attacker, victimIndex);
    monsterSteal(ctx, party, attacker, victimIndex);
}

// R38. Pre-attack, then attack (melee unless a spell was cast), then post-attack.
// A cast spell goes directly to Electrocute (post-attack step 1), skipping melee.
void resolveMonsterAttack(Ctx& ctx, Party& party, Creature& attacker, i32 victimIndex, bool castSpell) {
    monsterPreAttack(ctx, party, attacker, victimIndex);
    if (!castSpell) {
        (void)monsterMeleeAttempts(ctx, party, attacker, victimIndex);
    }
    monsterPostAttack(ctx, party, attacker, victimIndex);
}

// R39 LevelMod = Int(MonGuildLvl * LN(MonGuildLvl + 1))
i32 realLevelMod(i32 monGuildLvl) {
    const f64 m = static_cast<f64>(monGuildLvl);
    return static_cast<i32>(vbInt(m * std::log(m + 1.0)));
}

// R39 Modifier1 = Int((((LN(LevelMod) + LevelMod/15) / LN(1.08)) - CharGuildLvl/20) + 1)
i32 realBreathModifier(i32 monGuildLvl, i32 charGuildLvl) {
    const i32 levelMod = realLevelMod(monGuildLvl);
    // UNKNOWN: LN(LevelMod) is undefined for LevelMod <= 0 (monster levels 0-1); no modifier is used.
    if (levelMod <= 0) {
        return 0;
    }
    const f64 lm = static_cast<f64>(levelMod);
    const f64 v = ((std::log(lm) + lm / 15.0) / std::log(1.08)) - static_cast<f64>(charGuildLvl) / 20.0 + 1.0;
    return static_cast<i32>(vbInt(v));
}

// R39 second block: Modifier1 = Int((((LN(MonGuildLvl+1) + (MonGuildLvl+1)/15) / LN(1.08)) - CharGuildLvl/10) + 1)
i32 realAcidModifier(i32 monGuildLvl, i32 charGuildLvl) {
    const f64 mp = static_cast<f64>(monGuildLvl) + 1.0;
    const f64 v = ((std::log(mp) + mp / 15.0) / std::log(1.08)) - static_cast<f64>(charGuildLvl) / 10.0 + 1.0;
    return static_cast<i32>(vbInt(v));
}

// R39 breath base damage (one Rnd draw).
i32 breathBaseDamage(Ctx& ctx, i32 monGuildLvl, i32 charGuildLvl) {
    return breathBlockDamage(ctx, realBreathModifier(monGuildLvl, charGuildLvl));
}

// R39 acid base damage (one Rnd draw), second block.
i32 acidBaseDamage(Ctx& ctx, i32 monGuildLvl, i32 charGuildLvl) {
    return secondBlockDamage(ctx, realAcidModifier(monGuildLvl, charGuildLvl));
}

// R39 electrocution base damage (one Rnd draw), same block as acid.
i32 electrocutionBaseDamage(Ctx& ctx, i32 monGuildLvl, i32 charGuildLvl) {
    return secondBlockDamage(ctx, realAcidModifier(monGuildLvl, charGuildLvl));
}

// R40 step-up. The fallen front is removed and the next member is loaded.
// Load order (R28 as cited by R40): sign draw first, then amount draw.
// Hits = Def.Hits +/- swing, swing = Int(Rnd * (Def.Hits * 0.5) / 2) + 1. Max = loaded hits.
// UNKNOWN: Def.Hits is not stored separately; the creature's current maxHits is used as the base.
// UNKNOWN: the sign draw is taken as 50/50.
void stepUpFrontMember(Ctx& ctx, FoeGroup& group) {
    if (group.members.empty()) {
        return;
    }
    group.members.erase(group.members.begin());
    if (group.members.empty()) {
        return;
    }
    Creature& front = group.members[0];
    const i32 defHits = front.maxHits;
    const bool plus = ctx.rng.percent(50);
    const f64 amountDraw = ctx.rng.nextDouble();
    const i32 swing = static_cast<i32>(vbInt(amountDraw * (static_cast<f64>(defHits) * 0.5) / 2.0)) + 1;
    const i32 loaded = plus ? defHits + swing : defHits - swing;
    front.hits = loaded;
    front.maxHits = loaded;
}

// R40 / W-quirk. True when the aimed group's record shows it is empty (or the index is invalid).
bool aimedGroupGone(const SquareState& sq, const CombatOrder& order) {
    if (order.aimedGroup < 0 || order.aimedGroup >= kFoeGroupSlots) {
        return true;
    }
    return sq.foes[static_cast<u32>(order.aimedGroup)].count <= 0;
}

// R40 2. A thief reads the aimed group's record itself; if the group is empty it retargets.
void thiefCheckAimedGroup(Ctx& ctx, GameState& game, Character& thief, const SquareState& sq) {
    if (thief.activeGuild != Guild::Thief) {
        return;
    }
    if (aimedGroupGone(sq, thief.order)) {
        retarget(ctx, game, thief);
    }
}

// R41 1. A companion is released when its Hits > 0 and it is not ForLife (and it is occupied).
// UNKNOWN: the SRD test reads the companion's own Hits and ForLife; the owner is not needed.
bool fearReleases(const Character& owner, const Companion& comp) {
    (void)owner;
    return comp.occupied && comp.hits > 0 && !comp.forLife;
}

// R41. Fear trap (Trap 9). Releases eligible companions, clears the carry index, then
// announces when any companion fled. Returns the number that fled.
i32 fearTrap(Ctx& ctx, Party& party, Arr<Companion, kCompanionSlots>& slots) {
    i32 fled = 0;
    for (u32 s = 0; s < static_cast<u32>(kCompanionSlots); ++s) {
        Companion& comp = slots[s];
        if (!comp.occupied) {
            continue;
        }
        const i32 ownerIdx = ownerIndexOf(party, comp.owner);
        if (fearReleases(party.members[static_cast<u32>(ownerIdx)], comp)) {
            comp.occupied = false;
            comp.released = true;
            fled += 1;
        }
    }
    // R41 2. The carry reset applies even when the carried companion is dead and stays in its slot.
    if (party.carryingCompanion < kCompanionSlots) {
        party.carryingCompanion = -1;
    }
    // R41 3.
    if (fled > 0) {
        ctx.say("companion.fear", "Your companions were scared away!", {fled}, {});
    }
    return fled;
}

// W15. Average of NdM+K: n*(sides+1)/2 + plus. Examples: 4T6+15 -> 29, 3T6+6 -> 16.5, 2T6+6 -> 13.
f64 diceAverage(i32 n, i32 sides, i32 plus) {
    return static_cast<f64>(n) * static_cast<f64>(sides + 1) / 2.0 + static_cast<f64>(plus);
}

// W15. Rolls n dice of the given sides and adds plus. Each die is Int(Rnd*sides)+1.
// UNKNOWN: the SRD says printed values may be averages; this function rolls.
i32 rollDice(Ctx& ctx, i32 n, i32 sides, i32 plus) {
    i32 total = plus;
    for (i32 k = 0; k < n; ++k) {
        total += static_cast<i32>(ctx.rng.below(sides)) + 1;
    }
    return total;
}

// W16. has_ability = (flags & BIT) != 0. Bit values are UNKNOWN in this part.
bool hasFlagBit(u32 flags, u32 bit) {
    return (flags & bit) != 0u;
}

// W17. Floor n present if (Floor >> (n-1)) & 1 == 1.
// UNVERIFIED: hypothesis only (SRD c100). Valid for n in 1..32.
bool floorBitPresent(u32 floorMask, i32 n) {
    if (n < 1 || n > 32) {
        return false;
    }
    return ((floorMask >> static_cast<u32>(n - 1)) & 1u) == 1u;
}

}  // namespace mordor
