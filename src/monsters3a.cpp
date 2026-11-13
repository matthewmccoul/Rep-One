// monsters3a.cpp: implementation of the monsters3a module (SRD c11p3).
// Constants and declarations come from mordor/monsters3a.h.
// Unresolved SRD points are marked UNKNOWN: or CONFLICT: and resolved as stated.
#include "mordor/monsters3a.h"
#include "mordor/rng.h"

namespace mordor {

// R3: 150 seconds per level. Returns milliseconds.
i64 respawnBaseMs(i32 dungeonLevel) {
    return static_cast<i64>(kRespawnSecondsPerLevel) * static_cast<i64>(dungeonLevel) * 1000;
}

// R4: minus 5 s per empty area below header count. Returns milliseconds.
// UNKNOWN: R6 full sub0A91 formula; headerCount is not used in this part and
// the caller supplies emptyBelowHeader. Negative results are clamped to 0.
i64 respawnDelayMs(i32 dungeonLevel, i32 headerCount, i32 emptyBelowHeader) {
    (void)headerCount;
    // R50: sub0A91 (MONSMOD.BAS:698-700). Reference: DungeonState.cs RespawnDelay.
    // The empty-area term is headerAreas - tally and is not clamped: a tally above the
    // header count adds time, so a negative count must not be floored to zero.
    const i64 empty = static_cast<i64>(emptyBelowHeader);
    i64 delay = respawnBaseMs(dungeonLevel) -
                static_cast<i64>(kRespawnEmptyAreaPenaltySec) * 1000 * empty;
    // RESOLVED-BY-REFERENCE: DungeonState.cs RespawnDelay:
    // "if (t < 1200 / 10.0) t = 1200 / 10.0;" (120 seconds at least, not zero)
    const i64 kMinDelayMs = 120000;
    if (delay < kMinDelayMs) {
        delay = kMinDelayMs;
    }
    return delay;
}

// R8: stamp 0 reads as never set and refills on next visit.
// Port compares the absolute difference of the two times (c24).
// UNKNOWN: due when elapsed >= waitMs (boundary not stated in the SRD).
bool roomRefillDue(i64 emptiedStampMs, i64 nowMs, i64 waitMs) {
    if (emptiedStampMs == kRoomUnsetStamp) {
        return true;
    }
    i64 elapsed = nowMs - emptiedStampMs;
    if (elapsed < 0) {
        elapsed = -elapsed;  // port: absolute difference (c24)
    }
    return elapsed >= waitMs;
}

// R8: Foes(0).Ref == -1 marks an emptied room.
bool roomAlreadyEmptied(const Area& area) {
    return area.foes[0].ref == kEmptiedRoomRef;
}

// R8/R9: a clear does not stamp an already-empty room again.
// UNKNOWN: only the stamp and the Foes(0) marker are written; emptiedAt is not set here.
void stampRoomEmptied(Ctx& ctx, Area& area, i64 nowMs) {
    (void)ctx;
    if (roomAlreadyEmptied(area)) {
        return;
    }
    area.stamp = nowMs;  // a nowMs of 0 reads as never set (quirk, R8)
    area.foes[0].ref = kEmptiedRoomRef;
}

// R1/R9: class-pick double-count bug (kept per engine and tests).
// acc starts at v[0]; for each k the test is roll < acc, then v[k] is added,
// so the first class's weight is counted twice (10/80/10 gives about 10/10/80).
// Leftovers go to the last class.
// UNKNOWN: the draw range is taken as the weight total; empty or zero-total
// lists fall back to class 0 (c31).
i32 pickClassDoubleCount(Ctx& ctx, const Vec<i32>& weights) {
    const u32 n = static_cast<u32>(weights.size());
    if (n == 0) {
        return 0;  // UNKNOWN: empty fallback
    }
    i32 total = 0;
    for (u32 k = 0; k < n; ++k) {
        total += weights[k];
    }
    if (total <= 0) {
        return 0;  // UNKNOWN: zero-total fallback
    }
    const i32 roll = static_cast<i32>(ctx.rng.below(total));  // Int(Rnd*total)
    i32 acc = weights[0];
    for (u32 k = 0; k < n; ++k) {
        if (roll < acc) {
            return static_cast<i32>(k);
        }
        acc += weights[k];  // quirk: first class counted twice (c7/c31/c66)
    }
    return static_cast<i32>(n) - 1;  // leftovers to last class (c7)
}

// Class pick from spawn mask. Each set bit gives a class kind with weight 1,
// then pickClassDoubleCount applies the quirk.
// UNKNOWN: R11 zero mask is screened earlier or not; engine gives a random class
// when the list is empty (c7). R18: dungeonLevel is not used in this part.
i32 pickClassForMask(Ctx& ctx, i32 spawnMask, i32 dungeonLevel) {
    (void)dungeonLevel;
    constexpr i32 kClassKindCount = 18;  // R15: engine has 18 kinds (c13)
    Vec<i32> weights;
    Vec<i32> kinds;
    const u32 mask = static_cast<u32>(spawnMask);
    for (i32 bit = 0; bit < kClassKindCount; ++bit) {
        if (((mask >> static_cast<u32>(bit)) & 1u) != 0u) {
            weights.push_back(1);
            kinds.push_back(bit);
        }
    }
    if (weights.size() == 0) {
        // engine: empty class list gives a random class (c7); R11 UNKNOWN
        return static_cast<i32>(ctx.rng.below(kClassKindCount));
    }
    const i32 pick = pickClassDoubleCount(ctx, weights);
    return kinds[static_cast<u32>(pick)];
}

// R10: the 1% roll is always drawn. The newbie guard only blocks its effect (c7).
bool bypassRollPasses(Ctx& ctx, bool newbieGuardIsActive) {
    const bool roll = ctx.rng.percent(kBypassRollPercent);
    return roll && !newbieGuardIsActive;
}

// R12: character level 1 in areas below depth 18.
bool newbieGuardActive(i32 characterLevel, i32 dungeonLevel) {
    return characterLevel == kNewbieCharacterLevel && dungeonLevel < kNewbieDepthLimit;
}

// R12/R13: rerolls lead above guild level 2, except table index 4 (not monster Id 4).
bool newbieRerollsLead(bool newbieGuardIsActive, i32 leadGuildLevel, i32 classIndex) {
    return newbieGuardIsActive &&
           leadGuildLevel > kNewbieGuildLevelMax &&
           classIndex != kNewbieExemptClassIndex;
}

// R16: companion spawn mode mapping.
// 0..2 floor windows; 3 UNKNOWN (table index 3 assumed); 4 -> 4; 5 -> 5;
// 6 -> 4 (audit mapping, CONFLICT with header comment 4..6 = table index);
// 7 keeps previous window; above 7 -> mode - 7.
CompanionModeResult resolveCompanionMode(i32 companionSpawnMode) {
    CompanionModeResult r;
    r.kind = CompanionModeKind::FloorWindow;
    r.tableIndex = -1;
    if (companionSpawnMode >= 0 && companionSpawnMode <= kCompanionFloorWindowMax) {
        r.kind = CompanionModeKind::FloorWindow;
        r.tableIndex = -1;
    } else if (companionSpawnMode == kCompanionKeepWindowMode) {
        r.kind = CompanionModeKind::KeepWindow;
        r.tableIndex = -1;
    } else if (companionSpawnMode > kCompanionKeepWindowMode) {
        r.kind = CompanionModeKind::TableIndex;
        r.tableIndex = companionSpawnMode - kCompanionModeOffset;
    } else if (companionSpawnMode == 6) {
        // CONFLICT: R16 audit maps mode 6 to table index 4; header says 4..6 table index.
        r.kind = CompanionModeKind::TableIndex;
        r.tableIndex = 4;
    } else if (companionSpawnMode >= 3 && companionSpawnMode <= 5) {
        // UNKNOWN: mode 3 is not stated; treated as table index 3 with 4 and 5 mapping to themselves.
        r.kind = CompanionModeKind::TableIndex;
        r.tableIndex = companionSpawnMode;
    } else {
        // UNKNOWN: negative modes are not stated; falls back to floor window.
        r.kind = CompanionModeKind::FloorWindow;
        r.tableIndex = -1;
    }
    return r;
}

// Companion groups: every group up to groupCount is filled, clamped to kMaxGroups.
// UNKNOWN: the monster pick for each group is in another module; this function
// only sets livingCount from count. floorLevel is not used in this part (no ±1 floor).
void fillCompanionGroups(Ctx& ctx, Encounter& enc, i32 groupCount, i32 floorLevel) {
    (void)ctx;
    (void)floorLevel;
    i32 n = groupCount;
    if (n < 0) {
        n = 0;
    }
    if (n > kMaxGroups) {
        n = kMaxGroups;
    }
    for (i32 g = 0; g < n; ++g) {
        MonsterGroup& grp = enc.groups[static_cast<u32>(g)];
        grp.livingCount = grp.count;
    }
}

// R17: UNKNOWN between 150 (audit) and 300 (fidelity). 150 is used here.
bool companionSubtypeGivesUp(i32 attempts) {
    return attempts >= kCompanionSubtypeRetryLimit;
}

// R20: engine check elif lead.AlignmentValue==1, then the 40% gate.
// No viewer-alignment check (engine, c7). The draw happens only when the
// alignment check passes.
bool neutralFriendlyGate(Ctx& ctx, const Character& lead) {
    if (lead.alignment != Align::Neutral) {
        return false;
    }
    return ctx.rng.percent(kNeutralFriendlyPercent);
}

// R24: UNKNOWN which version is correct. Chosen: audit form with clamp at 0.
// CONFLICT: wiki and fidelity use <= / < without the clamp.
// The roll is 1..3000 and is always drawn. The threshold is CHA*10 - (levelFound-1)*10.
bool joinChanceRoll(Ctx& ctx, i32 charisma, i32 levelFound) {
    const i32 roll = static_cast<i32>(ctx.rng.below(kJoinRollSides)) + 1;
    i32 threshold = charisma * kJoinChaMultiplier - (levelFound - 1) * kJoinLevelMultiplier;
    if (threshold < 0) {
        threshold = 0;  // audit clamp (c23)
    }
    return roll <= threshold;
}

// R25: CONFLICT. Wiki says "exceeds", archive says "equal or greater".
// Archive tier chosen: characterGuildLevel >= monsterGuildLevel.
bool joinLevelEligible(i32 characterGuildLevel, i32 monsterGuildLevel) {
    return characterGuildLevel >= monsterGuildLevel;
}

// R26 and multi-group quirk: multi-group encounters never offer. Only group 0 is read (c25/c31/c66).
// UNKNOWN: a group is occupied when livingCount > 0.
bool encounterOffersJoin(const Encounter& enc) {
    i32 occupied = 0;
    for (i32 g = 0; g < kMaxGroups; ++g) {
        if (enc.groups[static_cast<u32>(g)].livingCount > 0) {
            ++occupied;
        }
    }
    if (occupied > 1) {
        return false;
    }
    return enc.groups[0].joinOffer;
}

// Joiners: only Foes(0).GroupCount can join. Joiners take the front's hits. Silent on success (c29).
// UNKNOWN: companion creation for joiners is done by the caller. Only the front
// group's counts are reduced here.
void admitJoiners(Ctx& ctx, Encounter& enc, i32 joinerCount) {
    (void)ctx;
    MonsterGroup& front = enc.groups[0];
    i32 n = joinerCount;
    if (n < 0) {
        n = 0;
    }
    if (n > front.count) {
        n = front.count;
    }
    front.count -= n;
    front.livingCount -= n;
    if (front.livingCount < 0) {
        front.livingCount = 0;
    }
}

// UNKNOWN: MonsterDef field name "id" is assumed from the SRD Id.
// Shared id 307 resolves to the first match. No match falls back to index 0 (c57/c62).
i32 monsterIndexForId(const GameData& data, i32 monsterId) {
    const u32 count = static_cast<u32>(data.monsters.size());
    for (u32 i = 0; i < count; ++i) {
        if (data.monsters[i].id == monsterId) {
            return static_cast<i32>(i);
        }
    }
    return 0;
}

// Asmodeus strip: the group count drops by one, not below one (c32/c46).
i32 groupCountAfterAsmodeusStrip(i32 groupCount) {
    const i32 stripped = groupCount - 1;
    if (stripped < 1) {
        return 1;
    }
    return stripped;
}

// Kills by a companion are on nobody's Kills (c37).
bool killCreditsCharacter(bool killedByCompanion) {
    return !killedByCompanion;
}

// Asmodeus charm: a charm kill raises the caster's kill count but adds no damage (c37).
// UNKNOWN: monsterKills record shape (creature id used). Experience is paid by the caller.
void recordCharmKill(Ctx& ctx, Character& caster, const Creature& creature) {
    (void)ctx;
    caster.kills += 1;
    caster.monsterKills.push_back(creature.id);
}

// Clear prune. Original: prune whenever UBound(Chars) > 0, i.e. party.size > 1 (FRMGROUP 326).
// CONFLICT: port prunes only when a member is able (party.anyAble). Original chosen.
// UNKNOWN: a group with no living monsters is pruned by setting count to 0.
void pruneGroupsOnClear(Ctx& ctx, Party& party, Encounter& enc) {
    (void)ctx;
    if (party.size <= 1) {
        return;
    }
    for (i32 g = 0; g < kMaxGroups; ++g) {
        MonsterGroup& grp = enc.groups[static_cast<u32>(g)];
        if (grp.livingCount <= 0) {
            grp.count = 0;
        }
    }
}

// Monster group loop exit. Original leaves only when UBound(Chars) = 0, i.e. a solo party (MULTIMOD 364).
// CONFLICT: port leaves on !Party.AnyAble. Original chosen.
bool monsterLoopExits(const Party& party) {
    return party.size == 1;
}

// Slay-class spell target. Original uses the first living group (MONSMOD 2561-2564).
// CONFLICT: port uses Foes(CombatOrders(c).M547A). Original chosen.
// UNKNOWN: -1 when no group is living.
i32 slayTargetGroup(const Encounter& enc) {
    for (i32 g = 0; g < kMaxGroups; ++g) {
        if (enc.groups[static_cast<u32>(g)].livingCount > 0) {
            return g;
        }
    }
    return -1;
}

// Initial KnownAs once per group (MONSMOD 1882-1946). Name table and index
// rounding are not in this part.
// UNKNOWN: name choice is not implemented; knownAs is left unchanged and no
// random draw is taken.
void rollGroupKnownAs(Ctx& ctx, MonsterGroup& group) {
    (void)ctx;
    (void)group;
}

// Breath of a guild-level 0 breather: original Log(0) error, group does nothing more that turn (c30).
// CONFLICT: port status differs. Original chosen.
bool breathGroupInert(i32 monsterGuildLevel) {
    return monsterGuildLevel == 0;
}

// Breath and acid test below 0. A companion brought to exactly 0 is not carried until the next sweep (c30).
bool breathAcidCarryTest(i32 hits) {
    return hits < 0;
}


// CONFLICT: penHasRoom(i32) is also declared in companions2a.h (API INDEX).
// Both declarations share the same signature. Only one translation unit may define it.
// Pen overflow cap on pen 0 is kPenCapacity (9). Strictly less than capacity means room.
bool penHasRoom(i32 occupancyPenZero) {
    return occupancyPenZero < kPenCapacity;
}

// Store restock buy-out gives up after kRestockEmptyDrawLimit (100) empty draws (c59).
bool restockBuyOutGivesUp(i32 emptyDraws) {
    return emptyDraws >= kRestockEmptyDrawLimit;
}

// While counting a chest, damage modifier gets +0.75 (c65).
f64 chestCountDamageBonus(bool countingChest) {
    return countingChest ? kChestCountDamageBonus : 0.0;
}

}  // namespace mordor
