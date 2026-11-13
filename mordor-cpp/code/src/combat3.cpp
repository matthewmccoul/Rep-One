#include "mordor/combat3.h"
#include "mordor/lookup.h"

namespace mordor {
namespace {

// UNKNOWN: ItemDef field names att, def and resistanceFlags are assumed.
// game_data.h was not supplied in full; adjust to the generated names.
const ItemDef* itemDefOf(const Ctx& ctx, const Item& item) {
    if (item.id < 0) {
        return nullptr;
    }
    return itemDefById(ctx.data, item.id);   // by id, not by table row
}

u64 resistIndex(DamageKind kind) {
    return static_cast<u64>(static_cast<i32>(kind));
}

// R2: "floored at zero".
i32 floorAtZero(i32 v) {
    return v < 0 ? 0 : v;
}

// R2: sum(equipped Att) over equipped gear.
// UNKNOWN: equipped set taken as worn items whose equipped flag is set.
i32 equippedAtt(const Ctx& ctx, const Inventory& inv) {
    i32 sum = 0;
    for (u64 i = 0; i < inv.worn.size(); ++i) {
        const Item& item = inv.worn[i];
        if (!item.equipped) {
            continue;
        }
        const ItemDef* def = itemDefOf(ctx, item);
        if (def != nullptr) {
            sum += def->att;
        }
    }
    return sum;
}

// R2: sum(equipped Def) over equipped gear.
i32 equippedDef(const Ctx& ctx, const Inventory& inv) {
    i32 sum = 0;
    for (u64 i = 0; i < inv.worn.size(); ++i) {
        const Item& item = inv.worn[i];
        if (!item.equipped) {
            continue;
        }
        const ItemDef* def = itemDefOf(ctx, item);
        if (def != nullptr) {
            sum += def->def;
        }
    }
    return sum;
}

// R11: alignment bit a wearer of this alignment matches on an item.
u32 wearerAlignBit(Align a) {
    switch (a) {
        case Align::Good:
            return kAlignBitGood;
        case Align::Neutral:
            return kAlignBitNeutral;
        case Align::Evil:
            return kAlignBitEvil;
    }
    return 0;
}

}  // namespace

// ---------------------------------------------------------------------------
// Attack and defence totals (R1-R4)
// ---------------------------------------------------------------------------

// R1: stat part = natural stat plus gear stat bonus, passed to StatsAD.
i32 statsADOf(const Character& c) {
    Arr<i32, kStatCount> withMods{};
    for (u64 i = 0; i < static_cast<u64>(kStatCount); ++i) {
        withMods[i] = c.abilities.stats[i] + c.abilities.statMods[i];
    }
    return statsAD(withMods);
}

// R2: AttackOf = max(0, Rules.BaseAD + StatsAD + BestGuildAD(c).att + sum(equipped Att)).
// The header takes const Ctx&, but baseAD needs Ctx&. A local copy of Ctx is
// made; it holds the same references, so rng and events are the same objects.
i32 attackOf(const Ctx& ctx, const Character& c, const Inventory& inv) {
    Ctx local(ctx);
    BestAd best = bestGuildAD(c);
    i32 total = baseAD(local, c) + statsADOf(c) + best.att + equippedAtt(ctx, inv);
    return floorAtZero(total);
}

// R2: DefenseOf = max(0, Rules.BaseAD + StatsAD + BestGuildAD(c).def + sum(equipped Def)).
i32 defenseOf(const Ctx& ctx, const Character& c, const Inventory& inv) {
    Ctx local(ctx);
    BestAd best = bestGuildAD(c);
    i32 total = baseAD(local, c) + statsADOf(c) + best.def + equippedDef(ctx, inv);
    return floorAtZero(total);
}

// R6: stored shift only counts while the stat part is held.
i32 storedADShift(const Character& c) {
    return statPartHeld(c) ? c.combat.storedAdShift : 0;
}

// R6: the stat part is held unless the character is paralysed.
// UNKNOWN: exact test for "stat part held".
bool statPartHeld(const Character& c) {
    return !c.ailments.paralysed;
}

// ---------------------------------------------------------------------------
// Defend (R8, R9)
// ---------------------------------------------------------------------------

// R9: the intended +50% Defend bonus is dead code in the original (overwritten
// before use). The port matches this: no bonus is applied.
i32 applyDefendBonus(i32 value, bool defendOn) {
    (void)defendOn;
    return value;
}

// ---------------------------------------------------------------------------
// Resistance (R10-R14)
// ---------------------------------------------------------------------------

// R10 step 1: race base. Zero if missing.
// UNKNOWN: RaceDef field name `resistances` assumed.
i32 raceResistBase(const Ctx& ctx, Race race, DamageKind kind) {
    u64 ri = static_cast<u64>(static_cast<i32>(race));
    if (ri >= ctx.data.races.size()) {
        return 0;
    }
    const RaceDef& def = ctx.data.races[ri];
    u64 k = resistIndex(kind);
    if (k >= def.resistances.size()) {
        return 0;
    }
    return static_cast<i32>(def.resistances[k]);
}

// R10 step 2 and R11: any counting equipped item has the kind's bit set.
bool countingItemResists(const Ctx& ctx, const Character& c, const Inventory& inv, DamageKind kind) {
    for (u64 i = 0; i < inv.worn.size(); ++i) {
        const Item& item = inv.worn[i];
        if (!item.equipped) {
            continue;
        }
        if (!countsForWearer(c, item)) {
            continue;
        }
        const ItemDef* def = itemDefOf(ctx, item);
        if (def == nullptr) {
            continue;
        }
        if (itemResistsKind(def->resistanceFlags, kind)) {
            return true;
        }
    }
    return false;
}

// R10 full: value = raceBase; counting item -> 90 (replace, R13); spell raise
// wins if higher.
// UNKNOWN: character spell resistance held in Resistances::resist.
i32 resistancePercentOf(const Ctx& ctx, const Character& c, const Inventory& inv, DamageKind kind) {
    i32 raceBase = raceResistBase(ctx, c.race, kind);
    bool counting = countingItemResists(ctx, c, inv, kind);
    i32 value = gearResistReplace(raceBase, counting);
    i32 spellRaise = c.resistances.resist[resistIndex(kind)];
    if (spellRaise > value) {
        value = spellRaise;
    }
    return value;
}

// R11: counting item = its alignment bit includes the wearer's alignment.
// UNKNOWN: exact test beyond "of your own alignment".
bool countsForWearer(const Character& c, const Item& item) {
    u32 bit = wearerAlignBit(c.alignment);
    return (static_cast<u32>(item.alignFlags) & bit) != 0;
}

// R12/R13: gear does not stack; the 90 replaces the race base.
i32 gearResistReplace(i32 raceBase, bool itemResists) {
    return itemResists ? 90 : raceBase;
}

// R14: item resistance bitmask test.
bool itemResistsKind(u32 resistanceFlags, DamageKind kind) {
    u64 idx = resistIndex(kind);
    if (idx >= static_cast<u64>(kResistKinds)) {
        return false;
    }
    return (resistanceFlags & kItemResistBits[idx]) != 0;
}

// ---------------------------------------------------------------------------
// Magic class to resistance type (R17)
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// Monster chance to hit (R18, R19)
// ---------------------------------------------------------------------------

// R18: archive article formula, clamped to [2, 98].
f64 monsterHitChance(i32 monAtk, i32 charDef) {
    f64 chance = kMonHitEqualChance;
    if (monAtk == charDef) {
        chance = kMonHitEqualChance;
    } else if (monAtk > charDef) {
        chance = kMonHitEqualChance +
                 kMonHitSpread * static_cast<f64>(monAtk - charDef) / static_cast<f64>(monAtk);
    } else {
        chance = kMonHitEqualChance -
                 kMonHitSpread * static_cast<f64>(charDef - monAtk) / static_cast<f64>(charDef);
    }
    return clampv(chance, kMonHitFloor, kMonHitCeiling);
}

// R18 roll.
// CONFLICT: fractional roll acceptance. Monster form used:
// Int(Rnd*100)+1 <= chance (accepts five values for 5.4).
bool monsterHitRoll(Ctx& ctx, i32 monAtk, i32 charDef) {
    f64 chance = monsterHitChance(monAtk, charDef);
    f64 roll = static_cast<f64>(ctx.rng.below(100) + 1);
    return roll <= chance;
}

}  // namespace mordor
