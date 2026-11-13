#include "mordor/spells1b.h"

namespace mordor {
namespace {

// R59 reach limit. UNKNOWN: MaxDungeonLevel's value is not given in these chunks.
// 15 is used because "deeper than 15" is the only level bound the SRD names.
constexpr i32 kFetchMaxDungeonLevel = 15;

i32 absI(i32 v) {
    return v < 0 ? -v : v;
}

const char* directionWord(RelativeDirection d) {
    switch (d) {
        case RelativeDirection::North: return "north";
        case RelativeDirection::NorthEast: return "north-east";
        case RelativeDirection::East: return "east";
        case RelativeDirection::SouthEast: return "south-east";
        case RelativeDirection::South: return "south";
        case RelativeDirection::SouthWest: return "south-west";
        case RelativeDirection::West: return "west";
        case RelativeDirection::NorthWest: return "north-west";
    }
    return "north";
}

}  // namespace

// R56. Ethereal Portal into rock.
// Port deviation: the original hangs in mdrcharSetDead's bare Resume; the port stops
// (returns) instead. This is the one deliberate deviation.
PortalOutcome castEtherealPortal(Ctx& ctx, Party& party, SquareState& square, i32 casterSlot) {
    // UNKNOWN: marking the square as rock needs SquareState fields, which are not in the
    // chunks. The square is therefore not modified here.
    (void)square;
    ctx.say("spell.portal.rock", "You walked into solid rock!", {}, {});

    if (casterSlot < kPortalPartyWipeSlotLimit) {
        // R56 slot 0 or 1: the whole party dies of rock.
        for (i32 i = 0; i < party.size; ++i) {
            party.members[static_cast<u32>(i)].deathState = DeathState::Rock;
        }
        party.wiped = true;
        return PortalOutcome::PartyRock;
    }

    // R56 slot 2 or later: the caster and every member after them die of rock.
    for (i32 i = casterSlot; i < party.size; ++i) {
        party.members[static_cast<u32>(i)].deathState = DeathState::Rock;
    }
    return PortalOutcome::CasterAndLaterRock;
}

// R57. Locate classification for one target.
LocateLine classifyLocate(const Location& from, const Location& to, i32 rosterIndex) {
    LocateLine line;
    line.rosterIndex = rosterIndex;
    // R57 outside LocateLevelRange.
    // UNKNOWN: range is measured as the absolute difference in dungeon level.
    if (absI(to.dungeonLevel - from.dungeonLevel) > kLocateLevelRange) {
        line.kind = LocateKind::TooFar;
        return line;
    }
    // R57 in the city.
    // UNKNOWN: the in-city test uses the area id (kCityArea).
    if (to.area == kCityArea) {
        line.kind = LocateKind::InCity;
        return line;
    }
    // R57 otherwise a relative direction is given.
    line.kind = LocateKind::Direction;
    line.direction = relativeDirection(from.x, from.y, to.x, to.y);
    return line;
}

// R57. Compass direction from one square to another.
// UNKNOWN: the SRD does not give sector boundaries or the sign convention for Y.
// Here y grows southward, a vertical sector needs |dy| >= 2|dx|, and a horizontal
// sector needs |dx| >= 2|dy|. The remaining cases are diagonal.
RelativeDirection relativeDirection(i32 fromX, i32 fromY, i32 toX, i32 toY) {
    const i32 dx = toX - fromX;
    const i32 dy = toY - fromY;
    if (dx == 0 && dy == 0) {
        return RelativeDirection::North;  // UNKNOWN: same square reports North.
    }
    const i32 ax = absI(dx);
    const i32 ay = absI(dy);
    const bool north = dy < 0;
    const bool east = dx > 0;
    if (ay >= 2 * ax) {
        return north ? RelativeDirection::North : RelativeDirection::South;
    }
    if (ax >= 2 * ay) {
        return east ? RelativeDirection::East : RelativeDirection::West;
    }
    if (north) {
        return east ? RelativeDirection::NorthEast : RelativeDirection::NorthWest;
    }
    return east ? RelativeDirection::SouthEast : RelativeDirection::SouthWest;
}

// R57. Lists every saved character except the caster.
void castLocate(Ctx& ctx, const Vec<Character>& roster, i32 casterIndex) {
    const i32 n = static_cast<i32>(roster.size());
    if (casterIndex < 0 || casterIndex >= n) {
        return;
    }
    const Character& caster = roster[static_cast<u32>(casterIndex)];
    for (i32 i = 0; i < n; ++i) {
        if (i == casterIndex) {
            continue;
        }
        const Character& other = roster[static_cast<u32>(i)];
        if (!other.saved) {
            continue;
        }
        const LocateLine line = classifyLocate(caster.location, other.location, i);
        switch (line.kind) {
            case LocateKind::TooFar:
                ctx.say("spell.locate.tooFar", "{0} is too far away to locate.", {}, {other.name});
                break;
            case LocateKind::InCity:
                ctx.say("spell.locate.inCity", "{0} is in the city.", {}, {other.name});
                break;
            case LocateKind::Direction:
                // strs carry the substitutions: {0} = name, {1} = direction word.
                ctx.say("spell.locate.direction", "{0} is to the {1}.",
                        {static_cast<i64>(line.direction)},
                        {other.name, Str(directionWord(line.direction))});
                break;
        }
    }
}

// R58. Lists dead characters whose cause is rock, on the leader's level, in area 0.
Vec<i32> fetchCandidates(const Vec<Character>& roster, const Party& party) {
    Vec<i32> out;
    const i32 leaderLevel = party.leader().location.dungeonLevel;
    const i32 n = static_cast<i32>(roster.size());
    for (i32 i = 0; i < n; ++i) {
        const Character& c = roster[static_cast<u32>(i)];
        if (c.isDead() && c.deathState == DeathState::Rock &&
            c.location.dungeonLevel == leaderLevel && c.location.area == kCityArea) {
            out.push_back(i);
        }
    }
    return out;
}

// R59. Reach test for a body's dungeon level.
// CONFLICT: R59 says refused "if that level is at or below MaxDungeonLevel", but R58 lists
// only bodies on the leader's level, and the "deeper than 15" half never fires. The literal
// text is used: within reach means the level is above kFetchMaxDungeonLevel.
bool isFetchWithinReach(i32 bodyDungeonLevel) {
    return bodyDungeonLevel > kFetchMaxDungeonLevel;
}

// R58 and R59. Cancel spends nothing. Otherwise the reach test applies, then the body moves.
FetchOutcome castFetch(Ctx& ctx, Party& party, Vec<Character>& roster, i32 pickedRosterIndex) {
    const i32 n = static_cast<i32>(roster.size());
    // R58 cancel: nothing spent, nothing moved.
    if (pickedRosterIndex < 0 || pickedRosterIndex >= n) {
        return FetchOutcome::Cancelled;
    }
    // UNKNOWN: a pick that is not in the R58 candidate list is treated as a cancel.
    const Vec<i32> candidates = fetchCandidates(roster, party);
    bool listed = false;
    const i32 candidateCount = static_cast<i32>(candidates.size());
    for (i32 j = 0; j < candidateCount; ++j) {
        if (candidates[static_cast<u32>(j)] == pickedRosterIndex) {
            listed = true;
        }
    }
    if (!listed) {
        return FetchOutcome::Cancelled;
    }

    Character& body = roster[static_cast<u32>(pickedRosterIndex)];
    // R59 refusal.
    if (!isFetchWithinReach(body.location.dungeonLevel)) {
        ctx.say("spell.fetch.notWithinReach", "{0} is not within reach.", {}, {body.name});
        return FetchOutcome::NotWithinReach;
    }
    // R59 retrieval: body moves to the caster's square and level, the leader's area, and is saved.
    // The caster's square is the party position.
    body.location.x = party.x;
    body.location.y = party.y;
    body.location.dungeonLevel = party.dungeonLevel;
    body.location.area = party.leader().location.area;
    body.saved = true;
    ctx.say("spell.fetch.retrieved", "{0} has been retrieved.", {}, {body.name});
    return FetchOutcome::Retrieved;
}

// R60. Lock-opening.
OpenLockOutcome castOpenLock(Ctx& ctx, Character& caster, SquareState& square, i32 effect,
                             i32 spellPointCost, i32 chestLockLevel) {
    // UNKNOWN: chest presence is taken as chestLockLevel > 0. SquareState fields are not in
    // the chunks, so the caller applies the unlock to the chest on Unlocked.
    (void)square;
    // R60 rule 1: no locked chest, nothing spent.
    if (chestLockLevel <= 0) {
        ctx.say("spell.openLock.noChest", "There is no locked chest here!", {}, {});
        return OpenLockOutcome::NoLockedChest;
    }
    // R60 rule 2: points are spent either way.
    caster.vitals.curSpellPoints -= spellPointCost;
    const i32 openUpTo = opensLockUpTo(effect);
    if (chestLockLevel <= openUpTo) {
        // R60 rule 2: lock level at or below the opening level unlocks.
        ctx.say("spell.openLock.unlocked", "The lock clicks open.", {static_cast<i64>(chestLockLevel)}, {});
        return OpenLockOutcome::Unlocked;
    }
    ctx.say("spell.openLock.stillLocked", "The lock holds.", {static_cast<i64>(chestLockLevel)}, {});
    return OpenLockOutcome::StillLocked;
}

// R61. Candidate: living companion (Hits > 0) with BindingLevel != 0.
bool isBindCandidate(const Companion& comp) {
    return comp.hits > 0 && comp.bindingLevel != 0;
}

// R61. Indices of all bind candidates.
Vec<i32> bindCandidates(const Vec<Companion>& companions) {
    Vec<i32> out;
    const i32 n = static_cast<i32>(companions.size());
    for (i32 i = 0; i < n; ++i) {
        if (isBindCandidate(companions[static_cast<u32>(i)])) {
            out.push_back(i);
        }
    }
    return out;
}

// R61. Binding (effects 40-42 and 150).
BindTargetOutcome castBinding(Ctx& ctx, Character& caster, Vec<Companion>& companions,
                              i32 pickedIndex, i32 effect, i32 spellPointCost) {
    const Vec<i32> candidates = bindCandidates(companions);
    // R61 none: Interact.NoBoundCompanions, nothing spent.
    if (candidates.size() == 0) {
        ctx.say("interact.noBoundCompanions", "There are no bound companions.", {}, {});
        return BindTargetOutcome::NoBoundCompanions;
    }
    // UNKNOWN: a pick that is not a candidate is treated as no bound companions, nothing spent.
    bool listed = false;
    const i32 candidateCount = static_cast<i32>(candidates.size());
    for (i32 j = 0; j < candidateCount; ++j) {
        if (candidates[static_cast<u32>(j)] == pickedIndex) {
            listed = true;
        }
    }
    if (!listed) {
        ctx.say("interact.noBoundCompanions", "There are no bound companions.", {}, {});
        return BindTargetOutcome::NoBoundCompanions;
    }

    // R61: points are spent, then the bind is applied.
    caster.vitals.curSpellPoints -= spellPointCost;
    Companion& comp = companions[static_cast<u32>(pickedIndex)];
    // UNKNOWN: SpellEffect.BindCompanion's body is not in the chunks. Only the bound flag is
    // set here, and effect 150 also sets forLife.
    comp.bound = true;
    if (effect == 150) {
        comp.forLife = true;
    }
    ctx.say("spell.bind.done", "{0} is bound.", {static_cast<i64>(effect)}, {comp.name});
    return BindTargetOutcome::Bound;
}

// R62. Fate. Fate always names the party's first member.
void applyFate(Ctx& ctx, Party& party) {
    // R62 step 1: one unused draw. c21 (rng.Next(3)) and c27 (l031E = Int(Rnd*3)) record the
    // same single draw.
    (void)ctx.rng.below(kFateUnusedDraw);
    // R62 step 2: reroll the first member's current hits and spell points.
    rerollFateVitals(ctx, party.members[0]);
}

// R62 step 2. Current hits, then current spell points, each uniform from 1 to its maximum.
void rerollFateVitals(Ctx& ctx, Character& who) {
    who.vitals.curHits = static_cast<i32>(ctx.rng.below(who.vitals.maxHits)) + 1;
    who.vitals.curSpellPoints = static_cast<i32>(ctx.rng.below(who.vitals.maxSpellPoints)) + 1;
    ctx.say("spell.fate.reroll", "{0} is re-rolled by Fate.",
            {static_cast<i64>(who.vitals.curHits), static_cast<i64>(who.vitals.curSpellPoints)},
            {who.name});
}

// R63. Stored point: SanctuaryX = X + 1; SanctuaryY = Y + 1; SanctuaryLevel = DungeonLevel.
SanctuaryPoint sanctuaryPointFor(const Party& party) {
    SanctuaryPoint p;
    p.x = party.x + 1;
    p.y = party.y + 1;
    p.dungeonLevel = party.dungeonLevel;
    return p;
}

// R63 set. Stores the caster's square as a 1-based point.
SanctuaryPoint castSetSanctuary(Ctx& ctx, const Party& party) {
    const SanctuaryPoint p = sanctuaryPointFor(party);
    ctx.say("spell.sanctuary.set", "Sanctuary is set here.",
            {static_cast<i64>(p.x), static_cast<i64>(p.y), static_cast<i64>(p.dungeonLevel)}, {});
    return p;
}

// R63 return. Zero stored point means never cast. Otherwise the caster is aged kSanctuaryDays.
// UNKNOWN: the SRD does not give the return's movement, so only the ageing is applied here.
SanctuaryOutcome castSanctuaryReturn(Ctx& ctx, Character& caster, const SanctuaryPoint& stored) {
    if (stored.isNever()) {
        ctx.say("spell.sanctuary.never", "No sanctuary has been set.", {}, {});
        return SanctuaryOutcome::NeverCast;
    }
    addAgeDays(caster, kSanctuaryDays);
    ctx.say("spell.sanctuary.returned", "{0} is returned to sanctuary.",
            {static_cast<i64>(kSanctuaryDays)}, {caster.name});
    return SanctuaryOutcome::Returned;
}

// R63 ageing (mdrcharAddAge). UNKNOWN: any side effect beyond ageDays is not in the chunks.
void addAgeDays(Character& who, i32 days) {
    who.ageDays += days;
}

// R64. Depth Perception and Find Direction end the lost state for the party.
void clearPartyLost(Ctx& ctx, Party& party) {
    party.lost = false;
    party.lostFacing = false;
    party.lostLevel = false;
    party.lostPosition = false;
    for (i32 i = 0; i < party.size && i < kPartyMaxSize; ++i) {
        party.members[static_cast<u32>(i)].lost = LostState{};
    }
    // UNKNOWN: message text is not given in the SRD.
    ctx.say("spell.lost.ended", "You are no longer lost.", {}, {});
}

// R64. Effects 11, 12 and 13.
void castUtilityEffect(Ctx& ctx, Party& party, FloorState& floor, UtilityEffect effect) {
    // UNKNOWN: Detect Rock's effect on FloorState is not in the chunks. The floor is not
    // modified here, and only the event is emitted.
    (void)floor;
    switch (effect) {
        case UtilityEffect::DetectRock:
            ctx.say("spell.utility.detectRock", "You sense the rock around you.", {}, {});
            break;
        case UtilityEffect::DepthPerception:
            ctx.say("spell.utility.depthPerception", "You perceive your depth.", {}, {});
            clearPartyLost(ctx, party);
            break;
        case UtilityEffect::FindDirection:
            ctx.say("spell.utility.findDirection", "You find your direction.", {}, {});
            clearPartyLost(ctx, party);
            break;
        default:
            break;
    }
}

}  // namespace mordor
