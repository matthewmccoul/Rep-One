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

namespace mordor {
namespace {

constexpr i32 kCompanionPickTries = 100;  // R27.3: up to 100 tries

inline u32 uidx(i32 i) {
    return static_cast<u32>(i);
}

f64 clampF64(f64 v, f64 lo, f64 hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

i32 clampIdx(i64 k, i32 count) {
    if (k < 0) return 0;
    if (k >= static_cast<i64>(count)) return count - 1;
    return static_cast<i32>(k);
}

i32 tableSize(const Ctx& ctx) {
    return static_cast<i32>(ctx.data.monsters.size());
}

// Mask bit test for classes/kinds 0..30 (R10, R22).
bool classBitSet(i32 mask, i32 cls) {
    if (cls < 0 || cls > 30) return false;
    return ((mask >> cls) & 1) != 0;
}

// UNKNOWN: placeholder for Fn0DEE (R30). The formula is not in this part; returns 0 damage.
i32 resolveMonsterBlow(i32 att, i32 charDef, i32 guildLevel, i32 str, f64 damMod) {
    (void)att;
    (void)charDef;
    (void)guildLevel;
    (void)str;
    (void)damMod;
    return 0;
}

}  // namespace

// R4: first table entry with this Id (Morgul at 278 precedes Shadow Ninja at 389).
// An Id with no creature falls back to index 0.
i32 monsterTableIndexForId(const Ctx& ctx, i32 monsterId) {
    const i32 n = tableSize(ctx);
    for (i32 i = 0; i < n; ++i) {
        if (ctx.data.monsters[uidx(i)].id == monsterId) return i;
    }
    return 0;
}

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

// R15: newbie lead guard. Lead index 4 is exempt (table index, not Id).
bool newbieLeadRejected(const Ctx& ctx, i32 leadIndex, i32 viewerLevel, i32 areaId) {
    if (leadIndex < 0 || leadIndex >= tableSize(ctx)) return false;
    const auto& lead = ctx.data.monsters[uidx(leadIndex)];
    return lead.guildLevel > kNewbieLeadMaxGuildLevel &&
           leadIndex != kNewbieExemptLeadIndex &&
           isNewbie(viewerLevel, areaId);
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

// R19: fogged groups (bit 20) start at class name (1); others at own name (0).
IdentifyLevel initialIdentifyLevel(u32 groupFlags) {
    return (groupFlags & kFogFlagBit) != 0 ? IdentifyLevel::CLASS_NAME : IdentifyLevel::OWN_NAME;
}

// R19: MonsterGroups[MonsterClasses[cls].Index].
Str monsterGroupName(const Ctx& ctx, i32 classIndex) {
    Str out{};
    if (classIndex < 0 || classIndex >= static_cast<i32>(ctx.data.monsterClasses.size())) return out;
    const i32 gi = ctx.data.monsterClasses[uidx(classIndex)].index;
    if (gi < 0 || gi >= static_cast<i32>(ctx.data.monsterGroups.size())) return out;
    out = ctx.data.monsterGroups[uidx(gi)].name;
    return out;
}

// R19: 0 = own name, 1 = class name, 2 = group name.
Str monsterDisplayName(const Ctx& ctx, i32 monsterIndex, i32 classIndex, IdentifyLevel level) {
    switch (level) {
        case IdentifyLevel::OWN_NAME: {
            Str out{};
            if (monsterIndex >= 0 && monsterIndex < tableSize(ctx)) {
                out = ctx.data.monsters[uidx(monsterIndex)].name;
            }
            return out;
        }
        case IdentifyLevel::CLASS_NAME: {
            Str out{};
            if (classIndex >= 0 && classIndex < static_cast<i32>(ctx.data.monsterClasses.size())) {
                out = ctx.data.monsterClasses[uidx(classIndex)].name;
            }
            return out;
        }
        case IdentifyLevel::GROUP_NAME:
            return monsterGroupName(ctx, classIndex);
    }
    return Str{};
}

// R20: misidentification.
MisidResult rollMisidentification(Ctx& ctx, f64 readerIdSkill, i32 guildLevel, i32 levelFound,
                                  i32 realIndex, const Vec<i32>& kin, ClassRange classRange) {
    MisidResult out{};
    out.misidentified = false;
    out.pickIndex = realIndex;

    // roll = Int(Rnd*100)+1 (always drawn).
    const i32 roll = static_cast<i32>(vbInt(ctx.rng.nextDouble() * 100.0)) + 1;

    // UNKNOWN: Log(0) is unguarded in the source; max(1, x) is used so the result is finite.
    const f64 logGuild = std::log(static_cast<f64>(guildLevel > 0 ? guildLevel : 1));
    const f64 skill = readerIdSkill - logGuild * static_cast<f64>(levelFound + 1);
    const f64 bar = clampF64(static_cast<f64>(kMisidBarBase) + skill * static_cast<f64>(kMisidBarPerSkill),
                             static_cast<f64>(kMisidBarMin), static_cast<f64>(kMisidBarMax));

    const i32 kinCount = static_cast<i32>(kin.size());
    if (!(static_cast<f64>(roll) > bar && kinCount > 1)) {
        return out;
    }

    // UNKNOWN: 'me' is taken as the position of the real monster in kin; kin holds table indices.
    i32 me = -1;
    for (i32 k = 0; k < kinCount; ++k) {
        if (kin[uidx(k)] == realIndex) {
            me = k;
            break;
        }
    }
    if (me < 0) return out;

    const f64 logFloor = std::log(static_cast<f64>(levelFound > 0 ? levelFound : 1));
    for (i32 attempt = 0; attempt < kMisidRedrawTries; ++attempt) {
        const i32 spread = static_cast<i32>(roundHalfEven(ctx.rng.nextDouble() * (logFloor + 2.5)));
        const i32 step = static_cast<i32>(vbInt(ctx.rng.nextDouble() * (static_cast<f64>(spread) + 1.5)));
        // CONFLICT: the text says the shift is upward when roll < bar, but this code only runs
        // when roll > bar. Literal reading: the shift is always downward.
        const bool up = static_cast<f64>(roll) < bar;
        const i32 pick = up ? me + step : me - step;
        if (pick >= 0 && pick < kinCount) {
            const i32 candidate = kin[uidx(pick)];
            if (candidate >= classRange.first && candidate <= classRange.last) {
                out.misidentified = (candidate != realIndex);
                out.pickIndex = candidate;
                return out;
            }
        }
    }
    return out;  // R20: 100 tries exhausted -> real monster
}

// R21: companion chain. UNKNOWN: the mode/subtype table (Tables section) is not in this part.
// A window with no weight restarts the whole encounter (c31), so this stub always restarts.
CompanionPick pickCompanionMonster(Ctx& ctx, const CompanionChain& previous, i32 targetFloor, i32 spread) {
    (void)ctx;
    (void)targetFloor;
    (void)spread;
    CompanionPick out{};
    out.restart = true;
    out.monsterIndex = -1;
    out.chain = previous;
    return out;
}

// R22: encounter floor. UNKNOWN: assumed to match the R12 formula (meLevel + 1 on STUD).
i32 encounterFloor(i32 level, bool stud) {
    return encounterTargetFloor(level, stud);
}

// R22 (test-stated): mask bit for the monster's kind, and LevelFound <= encounter floor.
// UNKNOWN: field 'kind' is assumed; the Kind table is not in the input.
// The 'stud == Floor.Has(STUD)' clause holds by construction, since the caller passes the square flag.
bool spawnEligible(const Ctx& ctx, const Area& area, i32 monsterIndex, bool stud, i32 viewerLevel) {
    if (monsterIndex < 0 || monsterIndex >= tableSize(ctx)) return false;
    const auto& m = ctx.data.monsters[uidx(monsterIndex)];
    if (!classBitSet(area.spawnMask, m.kind)) return false;
    return m.levelFound <= encounterFloor(viewerLevel, stud);
}

// R23: members behind the front are unrolled with max(1, base Hits).
// UNKNOWN: whether this is the final rule (see CONFLICT in the SRD).
i32 unrolledMemberHits(i32 baseHits) {
    return baseHits > 1 ? baseHits : 1;
}

// R25: a group with a live join offer does not roll.
// UNKNOWN: 'live' is taken as joinOffer == true.
bool groupRollsForEngagement(const MonsterGroup& group) {
    return !group.joinOffer;
}

// R33: target weights 100, 50, 25, 12 (four party slots).
// UNKNOWN: Party layout is not in the input; dead or absent members are not zeroed here.
Arr<i32, kPartyMaxSize> snapshotTargetWeights(const Party& party) {
    (void)party;
    Arr<i32, kPartyMaxSize> w{};
    w[0] = 100;
    w[1] = 50;
    w[2] = 25;
    w[3] = 12;
    return w;
}

// R27.2: target pick by weighted roll. Attempts = min(living, 5).
// CONFLICT: the text says a 1-100 roll, but the 53/27/13/7 split implies a roll over the total
// weight (187). The total is used.
i32 pickMonsterTarget(Ctx& ctx, const Arr<i32, kPartyMaxSize>& weights, const Party& party, i32 livingMembers) {
    (void)party;
    const i32 attempts = livingMembers < kMaxTargetAttempts ? livingMembers : kMaxTargetAttempts;
    i32 total = 0;
    for (i32 i = 0; i < kPartyMaxSize; ++i) {
        if (weights[uidx(i)] > 0) total += weights[uidx(i)];
    }
    for (i32 a = 0; a < attempts; ++a) {
        if (total <= 0) break;
        const i32 r = ctx.rng.below(total) + 1;
        i32 acc = 0;
        for (i32 i = 0; i < kPartyMaxSize; ++i) {
            acc += weights[uidx(i)];
            if (r <= acc) return i;
        }
    }
    // UNKNOWN: fallback after all attempts is -1 (no target).
    return -1;
}

// R34: draw under 100/(living companions+1) picks the character (strict).
bool meleeDrawPicksCharacter(Ctx& ctx, i32 livingCompanions) {
    const i32 living = livingCompanions < 0 ? 0 : livingCompanions;
    const f64 p = 100.0 / static_cast<f64>(living + 1);
    return ctx.rng.nextDouble() * 100.0 < p;
}

// R27.3: sample occupied living companion slots, up to 100 tries.
// Returns -1 to fall back to the character.
i32 pickCompanionSlot(Ctx& ctx, const Arr<Companion, kCompanionSlots>& companions) {
    // R41 / MONSMOD.BAS:2769-2778. With every slot empty there is no draw and the answer is 0.
    bool any = false;
    for (i32 s = 0; s < kCompanionSlots; ++s) {
        if (companions[uidx(s)].occupied) any = true;
    }
    if (!any) return 0;
    for (i32 guard = 0; guard < 1000; ++guard) {
        const i32 t = vbInt(ctx.rng.nextDouble() * 5.0);
        if (t == 0 || t == 4) return t;                   // the character, or the phantom fifth slot
        if (companions[uidx(t)].occupied) return t;       // occupied, alive or fallen
        // an empty slot reads -1 in the original, so draw again
    }
    // UNKNOWN: the reference gives up after 1000 draws and returns 0; the original's loop bound is not stated.
    return 0;
}

// R30 / R31: monster melee blow.
// UNKNOWN: Creature has no DamageMod field; 0 is passed, so the default 1 applies.
// UNKNOWN: Stats[0] is taken as the target's STR and charDef as the target's combat defence.
// UNKNOWN: rolls are crit then backstab (order not stated).
i32 monsterMeleeBlow(Ctx& ctx, const Creature& attacker, const Character& target, const Inventory& targetInv,
                     i32 sizeDiff, bool unseenInvisibleTarget) {
    (void)targetInv;
    const i32 ownerLevel = target.activeLevel();
    const bool crit = rollProc(ctx, procThreshold(kCritProcSpec, attacker.guildLevel, ownerLevel,
                                                  attacker.levelFound));
    const bool backstab = rollProc(ctx, procThreshold(kBackstabProcSpec, attacker.guildLevel, ownerLevel,
                                                      attacker.levelFound));
    const f64 damMod = meleeDamageModifier(0.0, target.openingChest, crit, backstab, sizeDiff,
                                           unseenInvisibleTarget);
    return resolveMonsterBlow(attacker.att, target.combat.defence, attacker.guildLevel,
                              target.abilities.stats[0], damMod);
}

// R27.5: accumulated character damage applied after the loop.
// UNKNOWN: Party layout is not in the input, so write-back is not implemented here.
void applyAccumulatedDamage(Ctx& ctx, Party& party, const Arr<i32, kPartyMaxSize>& damage) {
    (void)ctx;
    (void)party;
    (void)damage;
}

// R27.5: companion damage applied immediately.
// UNKNOWN: death threshold is not stated; hits < 0 is used, matching R36.
void applyCompanionDamage(Ctx& ctx, Companion& companion, i32 amount) {
    (void)ctx;
    companion.hits -= amount;
    if (companion.hits < 0) {
        companion.isDead = true;
        companion.deathState = CompanionDeathState::Dead;
    }
}

// R27.7: the first group draws its target once.
// UNKNOWN: the DeathState leave step (sub0E85) is not implemented here.
i32 loneBodyTargetDraw(Ctx& ctx, const Party& party) {
    const Arr<i32, kPartyMaxSize> w = snapshotTargetWeights(party);
    return pickMonsterTarget(ctx, w, party, 1);
}

// R27.8: continues while any monster stands, and not on IsOver.
bool roundPacedContinues(const Encounter& encounter, bool isOver) {
    if (isOver) return false;
    for (const auto& g : encounter.groups) {
        if (creatureAlive(g.front)) return true;
    }
    for (const auto& m : encounter.monsters) {
        if (creatureAlive(m)) return true;
    }
    return false;
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

// R28 audit: most = Int(Hits*0.5/2)+1.
i32 auditSwingBound(i32 baseHits) {
    return static_cast<i32>(vbInt(static_cast<f64>(baseHits) * 0.5 / 2.0)) + 1;
}

// R29: Alive = !Gone && (Hits > 0 || AtNought).
bool creatureAlive(const Creature& creature) {
    return !creature.gone && (creature.hits > 0 || creature.atNought);
}

// R29: any write to Hits clears AtNought.
void setCreatureHits(Creature& creature, i32 hits) {
    creature.hits = hits;
    creature.atNought = false;
}

// R30: melee blow modifier.
// CONFLICT: the audit adds -0.5 for an unseen invisible target (c23). The engine chunk (c18) does not.
// The flag is applied here as the header parameter implies.
f64 meleeDamageModifier(f64 monsterDamageMod, bool openingChest, bool crit, bool backstab,
                        i32 sizeDiff, bool unseenInvisibleTarget) {
    f64 damMod = monsterDamageMod > 0.0 ? monsterDamageMod : 1.0;
    if (openingChest) damMod += 0.75;
    if (crit) damMod += 2.0;
    if (backstab) damMod += 1.0;
    if (sizeDiff > 0) {
        damMod += static_cast<f64>(sizeDiff) / 10.0;
    } else if (sizeDiff < 0) {
        damMod -= static_cast<f64>(-sizeDiff) / 10.0;
    }
    if (unseenInvisibleTarget) damMod -= 0.5;
    return damMod;
}

// R31: threshold = clamp(100*ln(1+M/k)*ln(floor+1) - 100*ln(1+D/999), lo, hi).
// UNKNOWN: ln(0) for levelFound = -1 is not guarded; no monster has that value.
f64 procThreshold(const ProcSpec& spec, i32 attackerGuildLevel, i32 ownerGuildLevel, i32 levelFound) {
    const f64 m = static_cast<f64>(attackerGuildLevel);
    const f64 d = static_cast<f64>(ownerGuildLevel);
    const f64 floorV = static_cast<f64>(levelFound);
    const f64 raw = 100.0 * std::log(1.0 + m / spec.k) * std::log(floorV + 1.0)
                    - 100.0 * std::log(1.0 + d / kProcOwnerScale);
    return clampF64(raw, static_cast<f64>(spec.lo), static_cast<f64>(spec.hi));
}

// R31: companion melee defence bonus: +4 crit, +1 backstab.
// CONFLICT: monster melee uses +2/+1 and companion defence uses +4/+1 (c23). Both are kept.
i32 companionDefenceBonus(bool crit, bool backstab) {
    i32 bonus = 0;
    if (crit) bonus += 4;
    if (backstab) bonus += 1;
    return bonus;
}


// R35: fires if Floor(rnd()*100) < chance. The roll is drawn every time.
// UNKNOWN: the source's Log(0) error path for guild-level 0 breathers is not reproduced.
bool rollCompanionBreath(Ctx& ctx, i32 leadGuildLevel, i32 compLevel) {
    const i32 chance = companionBreathChance(leadGuildLevel, compLevel);
    const i64 roll = vbInt(ctx.rng.nextDouble() * 100.0);
    return roll < static_cast<i64>(chance);
}

// R36: acid never hits slot 0. It draws 1 + Int(Rnd*4), so it can return 4,
// the nonexistent fifth slot. Callers must treat index kCompanionSlots as no slot.
i32 rollAcidSlot(Ctx& ctx) {
    // R41 / MONSMOD.BAS:2769: raw draw Int(Rnd*5). 0 is the character, 4 is the phantom
    // slot nobody occupies. Empty companion slots 1..3 need the companions to detect,
    // so the full rule with its redraw is in pickCompanionSlot.
    return vbInt(ctx.rng.nextDouble() * 5.0);
}

// R36: breath and acid test below 0, so a companion at exactly 0 is not carried.
bool breathAcidRemoves(i32 hits) {
    return hits < 0;
}

}  // namespace mordor
