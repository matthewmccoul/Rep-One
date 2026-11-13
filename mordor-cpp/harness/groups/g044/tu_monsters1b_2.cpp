// monsters1b_2.cpp
// Joining and charm (R42-R46), clearing, refill and respawn (R47-R52), quest pick (R53),
// companion spawn mode (W2), paralysis part (W7). Each function is defined once overall
// (see monsters1b.cpp for the rest).
#include "mordor/monsters1b.h"

#include "mordor/combat3.h"
#include "mordor/math_util.h"

namespace mordor {

namespace {

constexpr i32 kFoeGroupSlots = 4;          // SquareState::foes is Arr<FoeGroup, 4>
constexpr i64 kSecondsPerDay = 86400;

// Atomic table read for quest tables. UNKNOWN: out-of-range reads are treated as 0.
i32 atOr(const Vec<i32>& v, i32 index) {
    if (index < 0 || static_cast<u32>(index) >= v.size()) {
        return 0;
    }
    return v[static_cast<u32>(index)];
}

// Maps the creature alignment onto the companion alignment (values differ in the two enums).
CompanionAlignment mapAlign(CharAlign a) {
    switch (a) {
        case CharAlign::Good:
            return CompanionAlignment::Good;
        case CharAlign::Neutral:
            return CompanionAlignment::Neutral;
        case CharAlign::Evil:
            return CompanionAlignment::Evil;
    }
    return CompanionAlignment::Good;
}

// Fills a companion slot with a joiner (R42 2): front's hits and rolled max.
// UNKNOWN: fields the SRD does not state (area, stats, chance to leave, level) are set simply.
void placeJoiner(Companion& slot, i32 slotIndex, i32 ownerIndex, const Creature& tmpl) {
    slot.occupied = true;
    slot.slot = slotIndex;
    slot.owner = ownerIndex;
    slot.released = false;
    slot.present = true;
    slot.dungeonLevel = tmpl.floor;
    slot.monsterId = tmpl.id;
    slot.ref = tmpl.ref;
    slot.name = tmpl.knownAs;
    slot.alignment = mapAlign(tmpl.align);
    slot.bindingLevel = kJoinBindingLevel;   // R42: -10
    slot.forLife = false;
    slot.bound = false;
    slot.joinedFreely = true;
    slot.hits = tmpl.hits;                   // R42 2: the front's hits
    slot.maxHits = tmpl.maxHits;             // R42 2: the rolled max
    slot.isDead = false;
    slot.deathState = CompanionDeathState::Normal;
    slot.pendingRaise = false;
    slot.poisonLevel = 0;
    slot.guildLevel = tmpl.guildLevel;
    slot.level = tmpl.levelFound;            // UNKNOWN: companion level source
    slot.chanceToLeave = 0;                  // UNKNOWN: chance-to-leave rule not in this part
    slot.carry = -1;
    slot.strikes = false;
    slot.specialPropertyFlags = tmpl.specialPropertyFlags;
}

i64 absI64(i64 v) {
    return v < 0 ? -v : v;
}

}  // namespace

// R42 1. A friendly group with JoinOffer > 0 offers to join.
bool joinOfferOpen(const FoeGroup& front) {
    return front.joinOffer > 0;
}

// R42 2-4. Joiners are taken from Foes(0) only: each takes the front's hits and rolled max,
// the front is never reloaded, and only GroupCount can join. With no free slot the message is
// given and the offer stands. Success is silent. Returns the number joined.
// UNKNOWN: joiners leave the group by decrementing count; members are not removed.
i32 recruitJoiners(Ctx& ctx, i32 ownerIndex, FoeGroup& front, Arr<Companion, kCompanionSlots>& slots) {
    if (front.members.empty()) {
        return 0;
    }
    const Creature tmpl = front.members[0];
    i32 joined = 0;
    while (front.count > 0) {
        i32 freeSlot = -1;
        for (u32 s = 0; s < static_cast<u32>(kCompanionSlots); ++s) {
            if (!slots[s].occupied) {
                freeSlot = static_cast<i32>(s);
                break;
            }
        }
        if (freeSlot < 0) {
            ctx.say("companion.noRoom", "Join - No room for more Companions!", {}, {});
            break;
        }
        placeJoiner(slots[static_cast<u32>(freeSlot)], freeSlot, ownerIndex, tmpl);
        front.count -= 1;
        joined += 1;
    }
    return joined;
}

// R42 AcceptJoin (key j). Offer must be open. Joiners are taken, then ClearedByJoining runs.
bool acceptJoin(Ctx& ctx, i32 ownerIndex, SquareState& sq, FloorState& floor,
                Arr<Companion, kCompanionSlots>& slots, i64 nowMs) {
    FoeGroup& front = sq.foes[0];
    if (!joinOfferOpen(front)) {
        return false;
    }
    const i32 joined = recruitJoiners(ctx, ownerIndex, front, slots);
    clearedByJoining(ctx, sq, floor, nowMs);
    return joined > 0;
}

// R42 5. If the group is emptied by joining, the room is cleared (MarkCleared path).
// UNKNOWN: the body beyond MarkCleared is not in this part; fight gold is not settled here.
void clearedByJoining(Ctx& ctx, SquareState& sq, FloorState& floor, i64 nowMs) {
    if (!noGroupRemains(sq)) {
        return;
    }
    markCleared(ctx, sq, floor, nowMs);
}

// R43. Threshold = CHA*10 - (LevelFound-1)*10.
i32 friendlyJoinThreshold(i32 cha, i32 levelFound) {
    return cha * 10 - (levelFound - 1) * 10;
}

// R43. Join if Int(Rnd*3000)+1 < threshold (engine, strict <).
// CONFLICT: audit and wiki forms use <= (1d3000 <= max(0, threshold)); the engine form is used.
bool friendlyJoinRoll(Ctx& ctx, i32 cha, i32 levelFound) {
    const i32 roll = static_cast<i32>(ctx.rng.below(kFriendlyJoinRollRange)) + 1;
    return roll < friendlyJoinThreshold(cha, levelFound);
}

// R43. count = Int(Rnd * min(groupSize, 4)) + 1.
i32 friendlyJoinCount(Ctx& ctx, i32 groupSize) {
    const i32 pool = clampv(groupSize, 1, kJoinGroupCap);
    return static_cast<i32>(ctx.rng.below(pool)) + 1;
}


// R44 offer eligibility.
// CONFLICT: wiki says the character's level must exceed the monster's level and the monster must
// be the only group; archive says the character's guild level must be >= the monster's guild
// level. The archive rule is used (simplest, single comparison).
bool monsterOffersJoin(const Character& ch, i32 monsterGuildLevel) {
    return ch.activeLevel() >= monsterGuildLevel;
}

// R44 charm percentage (wiki table): joinPct = max(0, (CHA - mLvl + 1) / 3).
f64 charmJoinPercent(i32 cha, i32 mLvl) {
    const f64 v = (static_cast<f64>(cha) - static_cast<f64>(mLvl) + 1.0) / 3.0;
    return maxv(0.0, v);
}

// R46. Asmodeus takes no portrait slot: count drops by one, not below one (group 0 only).
i32 asmodeusAdjustedCount(i32 monsterId, i32 count) {
    if (monsterId == kAsmodeusMonsterId && count > 0) {
        return maxv(1, count - 1);
    }
    return count;
}

// R46 quirk (charm). The charmed creature leaves the fight alive with its hits. The caster
// gets a kill but no damage.
// UNKNOWN: the experience formula for charm hits is not in this part; no experience is awarded.
void creditCharmedCreature(Ctx& ctx, Character& caster, Creature& creature, i64 hitsInflicted) {
    (void)hitsInflicted;
    creature.charmed = true;
    creature.engaged = false;   // UNKNOWN: leaving the fight is assumed to clear engagement
    caster.kills += 1;
    caster.monsterKills.push_back(creature.id);
    ctx.say("monster.charmed", "The creature is charmed and leaves the fight.", {creature.id}, {});
}

// Kills by a companion are on nobody's Kills (quirk, c37).
bool killCreditable(bool byCompanion) {
    return !byCompanion;
}

// A record left by a stone that is no longer a fight monster is nobody's damage (quirk, c37).
bool damageCreditable(bool fightMonster) {
    return fightMonster;
}

// True when every group in the room has count 0.
bool noGroupRemains(const SquareState& sq) {
    for (u32 g = 0; g < static_cast<u32>(kFoeGroupSlots); ++g) {
        if (sq.foes[g].count > 0) {
            return false;
        }
    }
    return true;
}

// R47 2. Gold is paid immediately when there is no chest; otherwise it waits in the chest.
// UNKNOWN: the SRD does not name the payee; the gold leader's on-hand purse is used.
void settleFightGold(Ctx& ctx, Party& party, SquareState& sq) {
    if (sq.fightGold == 0) {
        return;
    }
    if (sq.hasChest) {
        sq.chestGold += sq.fightGold;
        sq.fightGold = 0;
        return;
    }
    i32 payee = party.goldLeaderIndex;
    if (payee < 0 || payee >= party.size || payee >= kPartyMaxSize) {
        payee = 0;
    }
    party.members[static_cast<u32>(payee)].wallet.goldOnHand += sq.fightGold;
    ctx.say("monster.gold", "The party collects the fight gold.", {sq.fightGold}, {});
    sq.fightGold = 0;
}

// R47 1. Fight won and no group remains: MarkCleared runs, then gold is settled.
// UNKNOWN: the original asks the Asmodeus question first (MONSMOD 770-774). That question is
// not in this part and is not asked here.
void clearFight(Ctx& ctx, Party& party, SquareState& sq, FloorState& floor, i64 nowMs) {
    if (!noGroupRemains(sq)) {
        return;
    }
    markCleared(ctx, sq, floor, nowMs);
    settleFightGold(ctx, party, sq);
}

// R47 4. Standing groups older than 2t are removed on refill (R49 stale rule).
// UNKNOWN: a group with stamp 0 counts as stale (R49), so it is pruned too.
// UNKNOWN: the original prunes only when a party member is able; that check needs the party
// and is not applied here (quirk, c34).
void pruneStaleGroups(Ctx& ctx, SquareState& sq, i64 nowMs, i32 respawnSec) {
    (void)ctx;
    const i64 nowSec = secondsSinceMidnight(nowMs);
    for (u32 g = 0; g < static_cast<u32>(kFoeGroupSlots); ++g) {
        FoeGroup& fg = sq.foes[g];
        if (fg.count <= 0) {
            continue;
        }
        if (groupStale(nowSec, fg.stamp, respawnSec)) {
            fg.count = 0;
            fg.members.clear();
            fg.engaged = false;
            fg.joinOffer = 0;
        }
    }
}

// R48 MarkCleared. A room already marked empty (Foes[0].Ref == -1) is not stamped again.
// Clears all group counts and the front's join offer, sets Ref = -1 and Stamp = now (seconds
// since midnight), then Tally[level] = TallyOf(level) - 1.
// UNKNOWN: JoinOffer = 0 is applied to every group, not only Foes(0).
void markCleared(Ctx& ctx, SquareState& sq, FloorState& floor, i64 nowMs) {
    (void)ctx;
    FoeGroup& front = sq.foes[0];
    if (front.ref == -1) {
        return;
    }
    for (u32 g = 0; g < static_cast<u32>(kFoeGroupSlots); ++g) {
        sq.foes[g].count = 0;
        sq.foes[g].joinOffer = 0;
    }
    front.ref = -1;
    front.stamp = static_cast<i32>(secondsSinceMidnight(nowMs));
    floor.tally -= 1;
}

// R48 NoteRefilled: Tally[level] = TallyOf(level) + 1.
void noteRefilled(Ctx& ctx, FloorState& floor) {
    (void)ctx;
    floor.tally += 1;
}

// R49 nowSec. UNKNOWN: nowMs is taken as milliseconds since local midnight (VB Timer analogue).
// Values are not dated, so the same time the next day does not refill (quirk, c9).
i64 secondsSinceMidnight(i64 nowMs) {
    const i64 secs = (nowMs / kMsPerSecond) % kSecondsPerDay;
    return secs < 0 ? secs + kSecondsPerDay : secs;
}

// True when at least one group has count > 0 and a standing front member.
// UNKNOWN: "standing" is read from the group's first member.
bool anyStanding(const SquareState& sq) {
    for (u32 g = 0; g < static_cast<u32>(kFoeGroupSlots); ++g) {
        const FoeGroup& fg = sq.foes[g];
        if (fg.count > 0 && !fg.members.empty() && fg.members[0].standing) {
            return true;
        }
    }
    return false;
}

// R49 stale group: stamp == 0 (never set), or |nowSec - stamp| > 2t.
bool groupStale(i64 nowSec, i64 stampSec, i32 respawnSec) {
    if (stampSec == 0) {
        return true;
    }
    return absI64(nowSec - stampSec) > static_cast<i64>(kStaleWindowFactor) * respawnSec;
}

// R49 RefillDue:
//   empty = no standing groups
//   if not empty and (stale or stamp == 0): empty = true
//   return empty and (|nowSec - stamp| > t or stamp == 0)
bool refillDue(const SquareState& sq, i64 nowMs, i32 respawnSec) {
    const i64 nowSec = secondsSinceMidnight(nowMs);
    const i64 stamp = sq.foes[0].stamp;
    bool empty = !anyStanding(sq);
    if (!empty && groupStale(nowSec, stamp, respawnSec)) {
        empty = true;
    }
    if (!empty) {
        return false;
    }
    return stamp == 0 || absI64(nowSec - stamp) > static_cast<i64>(respawnSec);
}

// R49 IsOccupied = areaId > 0 and (RefillDue or AnyStanding).
bool isOccupied(const SquareState& sq, i64 nowMs, i32 respawnSec) {
    if (sq.areaId <= 0) {
        return false;
    }
    return refillDue(sq, nowMs, respawnSec) || anyStanding(sq);
}

// R50 engine respawn delay:
//   t = 1200 + (5 * (level / 2.0)) * 60   (evaluates to 1200 + 150*level)
//   t = t - (headerAreas - tally) * 5
//   if t < 120: t = 120
// CONFLICT: the test set (c66) uses (total - tally) as the operand; the engine form
// (headerAreas - tally, c9/c24/c31) is used here. The two arguments are passed as areaCount, tally.
i32 respawnDelaySeconds(i32 level, i32 areaCount, i32 tally) {
    i32 t = kRespawnBaseSeconds + kRespawnPerLevelSeconds * level;
    t = t - (areaCount - tally) * kRespawnPerAreaSeconds;
    if (t < kRespawnMinSeconds) {
        t = kRespawnMinSeconds;
    }
    return t;
}

// R53 level-gap gate: 90 if charLvl > monLvl + monLvl/4, 10 if charLvl < monLvl - monLvl/4, else 50.
i32 questLevelGapPercent(i32 charLvl, i32 monLvl) {
    const f64 quarter = static_cast<f64>(monLvl) / 4.0;
    if (static_cast<f64>(charLvl) > static_cast<f64>(monLvl) + quarter) {
        return kQuestGapHighPercent;
    }
    if (static_cast<f64>(charLvl) < static_cast<f64>(monLvl) - quarter) {
        return kQuestGapLowPercent;
    }
    return kQuestGapMidPercent;
}

// W2. Always spawns; percentage chance rolls once; only-if-space uses the space flag.
// UNKNOWN: numeric codes of the spawn modes are not in this part.
bool companionSpawns(Ctx& ctx, CompanionSpawnMode mode, i32 percent, bool spaceAvailable) {
    switch (mode) {
        case CompanionSpawnMode::Always:
            return true;
        case CompanionSpawnMode::PercentageChance:
            return ctx.rng.percent(percent);
        case CompanionSpawnMode::OnlyIfSpacePermits:
            return spaceAvailable;
    }
    return false;
}

// W7. Part = StatsADNow + StoredADShift. A full part (Str/Int/Wis/Dex 18) is kParalysisFullPart (32).
// The two terms are owned by combat3.
i32 paralysisPart(const Character& c) {
    return statsADNow(c) + storedADShift(c);
}

}  // namespace mordor

namespace mordor {
i32 shim_export_respawnDelaySeconds(i32 level, i32 areaCount, i32 tally) {
    return respawnDelaySeconds(level, areaCount, tally);
}
}  // namespace mordor
