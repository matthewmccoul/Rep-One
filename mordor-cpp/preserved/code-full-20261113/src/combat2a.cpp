// combat2a.cpp: chest traps (R1-R5, P1) and player crit/backstab procs (R6-R10).
#include "mordor/combat2a.h"
#include "mordor/combat3.h"
#include "mordor/math_util.h"

namespace mordor {

namespace {

// UNKNOWN: the CritChance/BackstabChance expressions are referenced in R7/R8 but not supplied.
// Placeholder clamps use the archive monster ranges (crit 2..50, backstab 4..75) [c99].
constexpr f64 kCritChanceMin = 2.0;
constexpr f64 kCritChanceMax = 50.0;
constexpr f64 kBackstabChanceMin = 4.0;
constexpr f64 kBackstabChanceMax = 75.0;

// UNKNOWN: fire resistance index is not identified in the supplied material.
constexpr i32 kFireResistIndex = -1;

// R2 trap numbers that print text in the original (P1 quirk).
constexpr i32 kSlimeTrapNumber = 5;
constexpr i32 kScaredTrapNumber = 9;

// R3 trap numbers that deal damage.
constexpr i32 kExplosionTrapNumber = 3;
constexpr i32 kFireTrapNumber = 4;

i32 trapNumber(ChestTrap trap) {
    return static_cast<i32>(trap);
}

f64 clampChance(f64 value, f64 lo, f64 hi) {
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

// R3 / P1 step 4: fire trap damage is reduced by fire resistance.
// UNKNOWN: the reduction formula and the fire resistance index are not supplied,
// so no reduction is applied until they are known.
f64 applyFireResistance(const Character& target, f64 dmg) {
    (void)target;
    if (kFireResistIndex < 0) {
        return dmg;
    }
    return dmg;
}

// P1 steps 4-5 for one party member.
void resolveMemberTrap(Ctx& ctx, Character& member, ChestTrap trap) {
    i32 n = trapNumber(trap);
    if (n == kExplosionTrapNumber || n == kFireTrapNumber) {
        f64 dmg = chestTrapDamage(ctx, trap, member);  // R3
        applyChestTrapDamage(member, dmg);             // R4 (and R5 inside hitsAfterChestTrap)
    }
    // R5: if CurHits < 1 then CurHits = 0. Applied after all damage (P1 step 5).
    if (member.vitals.curHits < 1) {
        member.vitals.curHits = 0;
    }
}

}  // namespace

// R1: trap selection.
// UNKNOWN: the selection method is not in the supplied material.
// Placeholder: uniform draw over 0..kChestTrapMax.
ChestTrap rollChestTrap(Ctx& ctx) {
    i32 pick = static_cast<i32>(ctx.rng.below(kChestTrapMax + 1));
    return static_cast<ChestTrap>(pick);
}

// R2: scope of a trap.
// Trap 0: none. Traps 1..4: whole party. Traps 5..10: opener only.
TrapScope chestTrapScope(ChestTrap trap) {
    i32 n = trapNumber(trap);
    if (n < 0 || n > kChestTrapMax) {
        return TrapScope::None;
    }
    if (n == 0) {
        return TrapScope::None;
    }
    if (n <= kChestTrapWholePartyMax) {
        return TrapScope::WholeParty;
    }
    return TrapScope::OpenerOnly;
}

// R3: damage for traps 3 (explosion) and 4 (fire). Other traps deal no damage.
// UNKNOWN: the damage formula is referenced as "per the Formulas section", which
// is not supplied. Placeholder damage is 0 until the formula is known.
f64 chestTrapDamage(Ctx& ctx, ChestTrap trap, const Character& target) {
    (void)ctx;
    i32 n = trapNumber(trap);
    if (n != kExplosionTrapNumber && n != kFireTrapNumber) {
        return 0.0;
    }
    f64 dmg = 0.0;  // UNKNOWN placeholder: base damage formula not supplied.
    if (n == kFireTrapNumber) {
        dmg = applyFireResistance(target, dmg);  // R3: trap 4 reduced by fire resistance.
    }
    return dmg;
}

// R4 and R5: returns the hit points after a chest trap.
// R4: CurHits = round_half_even(CurHits - dmg).
// R5: if CurHits < 1 then CurHits = 0.
i32 hitsAfterChestTrap(i32 curHits, f64 dmg) {
    i64 next = roundHalfEven(static_cast<f64>(curHits) - dmg);  // R4
    if (next < 1) {                                             // R5
        return 0;
    }
    constexpr i64 kI32Max = 2147483647;
    if (next > kI32Max) {
        return static_cast<i32>(kI32Max);
    }
    return static_cast<i32>(next);
}

// R4 applied to a character. R5 is applied by hitsAfterChestTrap.
void applyChestTrapDamage(Character& target, f64 dmg) {
    target.vitals.curHits = hitsAfterChestTrap(target.vitals.curHits, dmg);
}

// Chest traps 1 and 2 (reference ChestTraps.Spring): every party member is tested in order with
// Rnd*100 < ResistancePercent(kind)/2 (the polarity is reproduced as written) and, when caught, is poisoned
// (PoisonLevel) or diseased (DiseaseLevel) at the dungeon level. The resistance is the derived value
// (race base, counting gear, spell raise). The trap has no inventory in scope: an empty one means no gear.
// Returns a bit mask of the members afflicted.
u32 chestAfflictParty(Ctx& ctx, Party& party, DamageKind kind) {
    u32 mask = 0;
    for (i32 i = 0; i < party.size && i < kPartyMaxSize; ++i) {
        Character& c = party.members[static_cast<u32>(i)];
        const Inventory noGear{};
        const f64 roll = ctx.rng.nextDouble() * 100.0;
        const f64 threshold = static_cast<f64>(resistancePercentOf(ctx, c, noGear, kind)) / 2.0;
        if (roll < threshold) {
            const i32 level = c.location.dungeonLevel > 0 ? c.location.dungeonLevel : party.dungeonLevel;
            if (kind == DamageKind::Poison) {
                c.ailments.poisonLevel = level;
            } else {
                c.ailments.diseaseLevel = level;
            }
            mask |= 1u << static_cast<u32>(i);
        }
    }
    return mask;
}

// P1: chest trap, opener acts.
// Step 1 (monOpenChest) and step 2 (trap chosen) are performed by the caller.
void resolveChestTrap(Ctx& ctx, Party& party, ChestTrap trap) {
    // R38 / ch14 trap 1 (Poison Gas), as written. Each member in order is poisoned
    // when Rnd*100 < Resist(5)/2, at level = dungeon level. Nothing is printed.
    // The reference reproduces this polarity (ChestTraps.cs:58).
    if (trap == ChestTrap::PoisonGas || trap == ChestTrap::DiseaseSpores) {
        chestAfflictParty(ctx, party, trap == ChestTrap::PoisonGas ? DamageKind::Poison : DamageKind::Disease);
        return;
    }

    // Step 3: scope from R2.
    TrapScope scope = chestTrapScope(trap);
    if (scope == TrapScope::None) {
        return;
    }

    if (scope == TrapScope::WholeParty) {
        // Every party member, in member index order.
        for (i32 i = 0; i < party.size && i < kPartyMaxSize; ++i) {
            resolveMemberTrap(ctx, party.members[static_cast<u32>(i)], trap);
        }
    } else {
        // Opener only.
        i32 o = party.opener;
        if (o >= 0 && o < party.size && o < kPartyMaxSize) {
            resolveMemberTrap(ctx, party.members[static_cast<u32>(o)], trap);
        }
    }

    // Original quirk: only two trap arms print text.
    i32 n = trapNumber(trap);
    if (n == kSlimeTrapNumber) {
        // UNKNOWN: the item name is not supplied; placeholder text without it.
        ctx.say("combat.chest.slime", "SLIME: item Destroyed!", {}, {});
    }
    if (n == kScaredTrapNumber) {
        // UNKNOWN: trap 9 to text mapping is unconfirmed. The companion removal
        // effect is not specified, so only the message is emitted here.
        ctx.say("combat.chest.scared", "Your companions were scared away!", {}, {});
    }
}

// R7: crit chance as a percentage (roll is drawn from 0..99).
// R6: guild-level term plus weapon term, clamped to a floor and cap.
// UNKNOWN: the CritChance expression is not supplied. Placeholder:
// (critLevel - targetGuildLevel) + critMod + weaponCrit, clamped to 2..50.
f64 critChance(i32 critLevel, i32 critMod, i32 targetGuildLevel, i32 weaponFloor, i32 weaponCrit) {
    // RESOLVED-BY-REFERENCE: Rules.cs:390 CritChance -> SpecialHitChance(abilityDivisor 3, weaponLog 10,
    // floor 1, cap 50). The weapon floor term is used: fromWeapon = Log(weaponFloor+1) * Log(10), added only
    // when the weapon carries critical (item effect bit 4). Requires <cmath>.
    const f64 kCritAbilityDivisor = 3.0;
    const f64 kCritWeaponLog = 10.0;
    const f64 kCritFloor = 1.0;
    const f64 kCritCap = 50.0;
    const bool weaponHasIt = weaponCrit != 0;

    f64 p = ((std::log(static_cast<f64>(critLevel) / 999.0 + 1.0) * 100.0) *
             std::log(static_cast<f64>(critMod) + 1.0)) / kCritAbilityDivisor;
    p -= std::log(static_cast<f64>(targetGuildLevel) / 999.0 + 1.0) * 100.0;
    p /= 2.0;

    if (weaponHasIt) {
        f64 fromWeapon = std::log(static_cast<f64>(weaponFloor) + 1.0) * std::log(kCritWeaponLog);
        f64 sum = p + fromWeapon;
        p = (sum > fromWeapon) ? sum : fromWeapon;
    }
    // the floor only applies to someone who can do it at all: by guild level or by weapon
    if (p < kCritFloor && (critLevel > 0 || weaponHasIt)) {
        p = kCritFloor;
    }
    if (p > kCritCap) {
        p = kCritCap;
    }
    // The roll compares Int(Rnd*100) against this unrounded value, so the fraction counts.
    return p;
}

// R8: backstab chance as a percentage (roll is drawn from 0..99).
// UNKNOWN: the BackstabChance expression is not supplied. Placeholder:
// (backLevel - targetGuildLevel) + backMod + weaponBackstab, clamped to 4..75.
f64 backstabChance(i32 backLevel, i32 backMod, i32 targetGuildLevel, i32 weaponFloor, i32 weaponBackstab) {
    // RESOLVED-BY-REFERENCE: Rules.cs:403 BackstabChance -> SpecialHitChance(abilityDivisor 2, weaponLog 100,
    // floor 2, cap 75). The weapon floor term is used: fromWeapon = Log(weaponFloor+1) * Log(100), added only
    // when the weapon carries backstab (item effect bit 5). Requires <cmath>.
    const f64 kBackAbilityDivisor = 2.0;
    const f64 kBackWeaponLog = 100.0;
    const f64 kBackFloor = 2.0;
    const f64 kBackCap = 75.0;
    const bool weaponHasIt = weaponBackstab != 0;

    f64 p = ((std::log(static_cast<f64>(backLevel) / 999.0 + 1.0) * 100.0) *
             std::log(static_cast<f64>(backMod) + 1.0)) / kBackAbilityDivisor;
    p -= std::log(static_cast<f64>(targetGuildLevel) / 999.0 + 1.0) * 100.0;
    p /= 2.0;

    if (weaponHasIt) {
        f64 fromWeapon = std::log(static_cast<f64>(weaponFloor) + 1.0) * std::log(kBackWeaponLog);
        f64 sum = p + fromWeapon;
        p = (sum > fromWeapon) ? sum : fromWeapon;
    }
    // the floor only applies to someone who can do it at all: by guild level or by weapon
    if (p < kBackFloor && (backLevel > 0 || weaponHasIt)) {
        p = kBackFloor;
    }
    if (p > kBackCap) {
        p = kBackCap;
    }
    // The roll compares Int(Rnd*100) against this unrounded value, so the fraction counts.
    return p;
}

// R7/R8: rng.Next(100) < chance, using ctx.rng.below(kCritChanceScale).
bool rollProc(Ctx& ctx, f64 chancePercent) {
    f64 roll = static_cast<f64>(ctx.rng.below(kCritChanceScale));
    return roll < chancePercent;
}

// R7 then R8: crit is rolled before backstab. Both are rolled before the hit gate (R9).
// The hit gate itself is not applied here.
BlowProcs rollBlowProcs(Ctx& ctx, f64 critChancePercent, f64 backstabChancePercent) {
    BlowProcs procs;
    procs.crit = rollProc(ctx, critChancePercent);          // R7
    procs.backstab = rollProc(ctx, backstabChancePercent);  // R8
    return procs;
}

// R10: a crit adds +5 to DamMod and a backstab adds +2. Both apply when both succeed.
i32 blowModifierBonus(const BlowProcs& procs) {
    i32 bonus = 0;
    if (procs.crit) {
        bonus += kCritModBonus;       // R10
    }
    if (procs.backstab) {
        bonus += kBackstabModBonus;   // R10
    }
    return bonus;
}

// R10: both procs on one blow is "Severed".
bool blowIsSevered(const BlowProcs& procs) {
    return procs.crit && procs.backstab;
}

}  // namespace mordor
