// spells3_a.cpp: movement spell table lookups, stat requirements, and
// teleport-in-combat helpers for the spells3 module.
#include "mordor/spells3.h"
#include "mordor/ctx.h"
#include "mordor/math_util.h"
#include "mordor/state_character.h"
#include "mordor/state_creature.h"

#include <cmath>

namespace mordor {

namespace {

// Finds the row of kMovementSpellRows for a spell. Returns nullptr if absent.
const MovementSpellRow* findMovementRow(MovementSpell spell) {
    for (const MovementSpellRow& row : kMovementSpellRows) {
        if (row.spell == spell) {
            return &row;
        }
    }
    return nullptr;
}

// Sanctuary stat requirements, from the SRD table T-G "Req" column:
// 16 Int, 20 Wis, 15 Con.
inline constexpr i32 kSanctuaryReqInt = 16;
inline constexpr i32 kSanctuaryReqWis = 20;
inline constexpr i32 kSanctuaryReqCon = 15;

}  // namespace

// R2 / T-C, T-G: scope taken from the combatMarked flag of the spell row.
// Only Teleport is marked combat-usable in the table.
MovementScope movementScopeOf(MovementSpell spell) {
    const MovementSpellRow* row = findMovementRow(spell);
    if (row == nullptr) {
        return MovementScope::NonCombat;
    }
    return row->combatMarked ? MovementScope::CombatAndNonCombat
                             : MovementScope::NonCombat;
}

// R2 / T-G: base spell level from the movement spell table.
i32 movementSpellBase(MovementSpell spell) {
    const MovementSpellRow* row = findMovementRow(spell);
    if (row == nullptr) {
        return 0;
    }
    return row->base;
}

// T-G "Req" column. Only Sanctuary has a stat requirement in the SRD.
// UNKNOWN: the SRD lists no stat requirements for the other movement spells,
// so they are treated as always castable on stats.
// UNKNOWN: "Req" is read as the caster's current stats (abilities.stats),
// not statMax.
bool meetsMovementStatRequirements(const Character& caster, MovementSpell spell) {
    if (spell != MovementSpell::Sanctuary) {
        return true;
    }
    const i32 intel = caster.abilities.stats[static_cast<u32>(StatIndex::Int)];
    const i32 wis = caster.abilities.stats[static_cast<u32>(StatIndex::Wis)];
    const i32 con = caster.abilities.stats[static_cast<u32>(StatIndex::Con)];
    return intel >= kSanctuaryReqInt
        && wis >= kSanctuaryReqWis
        && con >= kSanctuaryReqCon;
}

// T-C / T-G Teleport: "in combat sends enemies into rock, based on magical
// resistance" [c83]. No formula is given in the SRD.
// UNKNOWN: simplest sensible choice: percent = 100 - magicResistance,
// clamped to 0..100.
i32 teleportImpalePercent(i32 creatureMagicResistance) {
    return clampv(100 - creatureMagicResistance, 0, 100);
}

// Teleport in combat (T-G, [c83]). The SRD gives no algorithm for targeting.
// Preferred structure taken from the TeleportCombatResult header contract:
//   sentAway = monsters removed from the encounter
//   impaled  = monsters sent into rock (subset of sentAway)
//   groupsHit = groups that lost at least one monster
// Caps: kTeleportCombatMaxMonsters total, kTeleportCombatMaxGroups groups.
// UNKNOWN: which monsters are targeted and in what order is not specified;
// this walks groups in index order.
// UNKNOWN: a removed monster is taken from the group by decrementing count and
// livingCount. The encounter.monsters vector is not modified because the
// mapping between it and the groups is not specified.
// UNKNOWN: one impale roll per removed monster, using the front creature's
// magical resistance.
TeleportCombatResult teleportCombatSendAway(Ctx& ctx, Encounter& encounter) {
    TeleportCombatResult result;

    const i32 groupLimit = minv(kTeleportCombatMaxGroups, kMaxGroups);

    for (i32 g = 0; g < groupLimit; ++g) {
        if (result.sentAway >= kTeleportCombatMaxMonsters) {
            break;
        }
        MonsterGroup& grp = encounter.groups[static_cast<u32>(g)];
        if (grp.livingCount <= 0) {
            continue;
        }

        const i32 pct = teleportImpalePercent(grp.front.magicResistance);
        bool hit = false;

        while (grp.livingCount > 0 && result.sentAway < kTeleportCombatMaxMonsters) {
            grp.livingCount -= 1;
            grp.count = maxv(0, grp.count - 1);
            result.sentAway += 1;
            hit = true;

            if (ctx.rng.percent(pct)) {
                result.impaled += 1;
            }
        }

        if (hit) {
            result.groupsHit += 1;
        }
    }

    ctx.say("spells3.teleport.combat",
            "Teleport in combat: monsters sent away, monsters impaled, groups hit",
            {result.sentAway, result.impaled, result.groupsHit},
            {});

    return result;
}

// R2 / T-I style damage modifier. The SRD half given does not contain the
// full damage formula. Only the sheet pieces are documented:
//   log term:   LN(SL) / LN(5)         [c74 quirks, damage sheet]
//   power term: INT((SL/10) * (H - 1)) [c78, c80]; H = 1 gives zero
// Here power plays the role of H.
// UNKNOWN: how base enters the modifier is not in this SRD part. base is
// accepted for the signature but not used.
// UNKNOWN: sl <= 0 is guarded to 0.0 because LN is undefined there. The SRD
// does not state this guard.
double damageModifier1(i32 base, i32 sl, i32 power) {
    // R6 step 1 (ch10): Modifier1 = Base + Base*LN(SL+1)/LN(5) + power term.
    // CONFLICT: SRD writes the power term as Int(...) (floor). Reference Rules.SpellDamage
    // (Rules.cs:628) uses the C# (int) cast, which truncates toward zero. Reference preferred.
    // The two differ only when (SL/10)*(power-1) is negative (power 0).
    // RESOLVED-BY-REFERENCE: Rules.SpellDamage (Rules.cs:624-628) runs `sl = Math.Max(1, sl);`
    // before Modifier1, so SL below 1 is treated as 1 in both the log term and the power term.
    // The SRD is silent on SL < 1.
    const i32 level = sl < 1 ? 1 : sl;
    const double b = static_cast<double>(base);
    const double slD = static_cast<double>(level);

    const double logTerm = b * std::log(slD + 1.0) / std::log(5.0);

    // Truncation toward zero, matching the reference (int) cast.
    const double powerRaw = (slD / 10.0) * static_cast<double>(power - 1);
    const double powerTerm = static_cast<double>(static_cast<i64>(powerRaw));
    return b + logTerm + powerTerm;
}

}  // namespace mordor
