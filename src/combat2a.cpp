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

}  // namespace

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

}  // namespace mordor
