// monsters1a.cpp: implements include/mordor/monsters1a.h (SRD c11p1, rules R1-R36).
//
// UNKNOWN: game_data.h is generated and not in the input. Field names are assumed to be
//   camelCase of the SRD names: MonsterDef{id, type, levelFound, guildLevel, chance, hits,
//   att, kind, name}; monsterClasses[]{index, name}; monsterGroups[]{name} (NamedEntry).
// CONFLICT: rollProc(Ctx&, f64) is declared in monsters1a.h and in combat2a.h (API INDEX
//   assigns it to combat2a). It is NOT defined here, to avoid a duplicate symbol.
// UNKNOWN: Party layout is not in the input. Functions that touch Party do not read its fields.
// UNKNOWN: Fn0DEE (the R30 blow resolver) has no formula in this part; resolveMonsterBlow is a placeholder.

#include <cmath>
#include <cstddef>

#include "mordor/ctx.h"
#include "mordor/game_data.h"
#include "mordor/math_util.h"
#include "mordor/monsters1a.h"
#include "mordor/rng.h"
#include "mordor/state_character.h"
#include "mordor/state_companion.h"
#include "mordor/state_creature.h"
#include "mordor/state_dungeon.h"
#include "mordor/types.h"
#include "mordor/shared_helpers.h"

namespace mordor {
namespace {

constexpr i32 kCompanionPickTries = 100;  // R27.3: up to 100 tries


f64 clampF64(f64 v, f64 lo, f64 hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

i32 tableSize(const Ctx& ctx) {
    return static_cast<i32>(ctx.data.monsters.size());
}

// Mask bit test for classes/kinds 0..30 (R10, R22).
bool classBitSet(i32 mask, i32 cls) {
    if (cls < 0 || cls > 30) return false;
    return ((mask >> cls) & 1) != 0;
}

}  // namespace

// R8: preconditions.
EncounterPrecheck encounterPrecheck(i32 squareCode, i32 areaId, bool hasStoredEncounter, i32 spawnMask) {
    // R8.1: no encounter on a rock square or in area 0.
    if (squareCode == kRockSquareId || areaId == kNoEncounterAreaId) {
        return EncounterPrecheck::Blocked;
    }
    // R8.2: a positive area id with a stored encounter spawns.
    if (areaId > 0 && hasStoredEncounter) {
        return EncounterPrecheck::UseStored;
    }
    // R8.3 RESOLVED-BY-REFERENCE: a zero spawn mask gives no encounter, before any class pick.
    // Reference: "if (mask == 0) return e;" (Combat.cs Generate, MONSMOD.BAS:1448-1452).
    if (spawnMask == 0) {
        return EncounterPrecheck::Blocked;
    }
    return EncounterPrecheck::Roll;
}

// R9: lair roll.
LairRoll rollLair(Ctx& ctx, const Area& area) {
    LairRoll out{};
    // UNKNOWN: VB 'And' may evaluate RndF unconditionally; short-circuit (C++) is used here.
    if (area.lairId > 0 &&
        ctx.rng.nextDouble() * 100.0 <= static_cast<f64>(kLairChancePercent)) {
        const i32 n = tableSize(ctx);
        i32 lead = -1;
        for (i32 i = 0; i < n; ++i) {
            if (ctx.data.monsters[uidx(i)].id == area.lairId) {
                lead = i;
                break;
            }
        }
        if (lead < 0) {
            out.outcome = LairOutcome::Failed;  // R9: lair Id not in table -> encounter fails
            return out;
        }
        out.outcome = LairOutcome::Lead;
        out.leadIndex = lead;
        return out;
    }
    out.outcome = LairOutcome::NotTaken;
    return out;
}

// R10 ClassCount: MonsterClasses.Count if > 0, else max(m.Type)+1.
i32 monsterClassCount(const Ctx& ctx) {
    const i32 declared = static_cast<i32>(ctx.data.monsterClasses.size());
    if (declared > 0) return declared;
    i32 maxType = -1;
    for (const auto& m : ctx.data.monsters) {
        if (m.type > maxType) maxType = m.type;
    }
    return maxType + 1;
}

// R10: newbie = meLevel == 1 and areaId < 18.
bool isNewbie(i32 viewerLevel, i32 areaId) {
    return viewerLevel == kNewbieLevel && areaId < kNewbieMaxAreaId;
}

// R10: class pick (lair is handled by the caller).
i32 pickEncounterClass(Ctx& ctx, const Area& area, i32 viewerLevel, i32 areaId, const Vec<ClassRange>& ranges) {
    const i32 nc = monsterClassCount(ctx);
    const bool newbie = isNewbie(viewerLevel, areaId);

    // R10: the 1% any-class roll is always drawn; the newbie check only blocks its effect.
    // CONFLICT: fidelity says the roll is suppressed for newbies (c31/c34). Engine order is followed.
    const i32 anyRoll = static_cast<i32>(vbInt(ctx.rng.nextDouble() * 100.0)) + 1;
    if (anyRoll < kAnyClassRollBelow && !newbie) {
        return ctx.rng.below(nc);
    }

    Vec<i32> list;
    Vec<i32> weights;
    const i32 nr = static_cast<i32>(ranges.size());
    for (i32 c = 0; c < nc && c < nr; ++c) {
        if (!classBitSet(area.spawnMask, c)) continue;
        const ClassRange& rg = ranges[uidx(c)];
        if (rg.first == -1) continue;
        // UNKNOWN: class weight = sum of Chance over the class's table range (R11 'eligible m').
        i32 w = 0;
        for (i32 i = rg.first; i <= rg.last; ++i) {
            w += ctx.data.monsters[uidx(i)].chance;
        }
        list.push_back(c);
        weights.push_back(w);
    }
    if (list.empty()) {
        return ctx.rng.below(nc);  // R10: empty list -> uniform class
    }
    return pickWeightedClass(ctx, list, weights, nc);
}

// R11: weighted class pick, including the documented double-count quirk.
i32 pickWeightedClass(Ctx& ctx, const Vec<i32>& list, const Vec<i32>& weights, i32 classCount) {
    const i32 count = static_cast<i32>(list.size());
    if (count == 0) {
        return ctx.rng.below(classCount);  // R10 empty-list rule
    }
    f64 total = 0.0;
    for (i32 k = 0; k < count; ++k) {
        total += static_cast<f64>(weights[uidx(k)]);
    }
    if (total == 0.0) {
        // R11 fallback: cls = list[CInt(Rnd*(nc-2))]. No RndF draw on this branch.
        // RESOLVED-BY-REFERENCE: the reference list has nc slots and the unfilled ones hold 0 (class 0),
        // so an index past the filled entries names class 0. Reference: "unfilled slots stay 0, which is class 0"
        // and "const draw lands on any of the first NumMonsterClasses - 1 slots" (Combat.cs, cls = list[CInt(Rnd() * (nc - 1 - 1))]).
        const i64 k = roundHalfEven(ctx.rng.nextDouble() * static_cast<f64>(classCount - 2));
        if (k < 0 || k >= static_cast<i64>(count)) {
            return 0;
        }
        return list[uidx(static_cast<i32>(k))];
    }
    const f64 r = ctx.rng.nextDouble() * 101.0;  // r = RndF()*101, drawn only on the weighted branch
    // Literal loop: acc starts at v[0] and v[k] is added after each failed test.
    f64 acc = static_cast<f64>(weights[uidx(0)]);
    for (i32 k = 0; k < count; ++k) {
        const f64 thr = acc / total * 100.0;
        if (r <= thr) {
            return list[uidx(k)];
        }
        acc += static_cast<f64>(weights[uidx(k)]);
    }
    // Leftovers go to the last class (R11).
    return list[uidx(count - 1)];
}


// R12: LevelFound in [m0282 - spread, m0282].
bool inEligibilityWindow(i32 levelFound, i32 targetFloor, i32 spread) {
    return levelFound >= targetFloor - spread && levelFound <= targetFloor;
}

// R13: pick within the class by cumulative weight among eligible entries.
// Returns found=false when no eligible entry has weight; the caller widens spread (R13.1).
// UNKNOWN: exact draw form is not stated in this part; RndF()*total is used.
MonsterPick pickMonsterInClass(Ctx& ctx, ClassRange range, i32 targetFloor, i32 spread) {
    MonsterPick out{};
    if (range.first < 0 || range.last < range.first) return out;

    i32 total = 0;
    for (i32 i = range.first; i <= range.last; ++i) {
        const auto& m = ctx.data.monsters[uidx(i)];
        if (inEligibilityWindow(m.levelFound, targetFloor, spread)) {
            total += m.chance;
        }
    }
    if (total <= 0) return out;  // R13.1

    const f64 r = ctx.rng.nextDouble() * static_cast<f64>(total);
    f64 acc = 0.0;
    i32 lastEligible = -1;
    for (i32 i = range.first; i <= range.last; ++i) {
        const auto& m = ctx.data.monsters[uidx(i)];
        if (!inEligibilityWindow(m.levelFound, targetFloor, spread) || m.chance <= 0) continue;
        lastEligible = i;
        acc += static_cast<f64>(m.chance);
        if (r < acc) {
            out.found = true;
            out.monsterIndex = i;
            return out;
        }
    }
    // Float rounding fallback: last eligible entry.
    out.found = lastEligible >= 0;
    out.monsterIndex = lastEligible;
    return out;
}

// R13.2: encounter restarts when spread goes past 3.
bool spreadExhausted(i32 spread) {
    return spread > kSpreadRestartLimit;
}

// R14: first and last table index per class. Scan starts at i = 1 (quirk).
Vec<ClassRange> buildClassRanges(const Ctx& ctx) {
    const i32 nc = monsterClassCount(ctx);
    const i32 n = tableSize(ctx);
    Vec<ClassRange> r;
    r.resize(static_cast<std::size_t>(nc));
    for (i32 c = 0; c < nc; ++c) {
        ClassRange& rc = r[uidx(c)];
        for (i32 i = 1; i < n; ++i) {
            const i32 t = ctx.data.monsters[uidx(i)].type;
            if (rc.first == -1 && t == c) {
                rc.first = i;
            }
            if (t != c && rc.first > -1) {
                rc.last = i - 1;
                break;
            }
        }
    }
    // R14: unconditional final assignment.
    if (nc > 0) {
        r[uidx(nc - 1)].last = n - 1;
    }
    return r;
}

// R16: group count from the lead's NumGroups, clamped to 1..4.
i32 rollGroupCount(Ctx& ctx, i32 numGroups) {
    i32 groups = numGroups;
    // Int(Rnd*100) < groups/(4*2)*100 (always drawn).
    const f64 chance = static_cast<f64>(groups) / (4.0 * 2.0) * 100.0;
    const f64 first = static_cast<f64>(vbInt(ctx.rng.nextDouble() * 100.0));
    if (first < chance) {
        const i64 cut = vbInt(ctx.rng.nextDouble() * (static_cast<f64>(numGroups) / 2.0 + 1.0));
        groups = groups - static_cast<i32>(cut);
    }
    return static_cast<i32>(clampF64(static_cast<f64>(groups),
                                     static_cast<f64>(kGroupCountMin),
                                     static_cast<f64>(kMaxGroups)));
}

// R17: group size.
i32 rollGroupSize(Ctx& ctx, i32 inGroup) {
    const i64 cut = roundHalfEven(ctx.rng.nextDouble() * (static_cast<f64>(inGroup) / 2.0));
    i32 groupCount = inGroup - static_cast<i32>(cut);
    if (groupCount < 1) groupCount = 1;
    return groupCount;
}

// R18: group 0 hostility. Returns true when engaged (hostile).
bool rollLeadEngaged(Ctx& ctx, i32 leadAlignmentValue, Align viewerAlign) {
    // Good lead vs Good viewer: peaceful, no draw.
    if (leadAlignmentValue == 0 && viewerAlign == Align::Good) {
        return false;
    }
    // Neutral lead: Engaged = (Int(Rnd*100) < 40) ? 0 : 1.
    if (leadAlignmentValue == 1) {
        const i64 roll = vbInt(ctx.rng.nextDouble() * 100.0);
        return !(roll < kNeutralPeacePercent);
    }
    // UNKNOWN: Evil lead alignment code is not given in this part; default Engaged = 1.
    return true;
}

// R23: members behind the front are unrolled with max(1, base Hits).
// UNKNOWN: whether this is the final rule (see CONFLICT in the SRD).
i32 unrolledMemberHits(i32 baseHits) {
    return baseHits > 1 ? baseHits : 1;
}

// R28: monster hits roll on load. sign = Int(Rnd*2); swing = Int(Rnd*(Hits*.5)/2)+1.
// No floor is applied (a 1-hit creature gives 0 or 2).
RolledHits rollMonsterHits(Ctx& ctx, i32 baseHits) {
    const i32 sign = ctx.rng.below(2);
    const i32 swing = static_cast<i32>(vbInt(ctx.rng.nextDouble() * (static_cast<f64>(baseHits) * 0.5) / 2.0)) + 1;
    RolledHits out{};
    out.hits = (sign == 0) ? baseHits - swing : baseHits + swing;
    out.maxHits = out.hits;
    return out;
}

}  // namespace mordor
