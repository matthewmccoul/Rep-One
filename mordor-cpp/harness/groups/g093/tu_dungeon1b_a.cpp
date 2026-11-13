// src/dungeon1b_a.cpp
// Module dungeon1b, half a: SRD c15p1, rules R56-R88
// (walked and taken chutes, water and ticks, teleporter squares,
// Teleport / displacement / Ethereal Portal / Soul Search).
//
// UNKNOWN: GameData field names are assumed, since game_data.h is not in the brief:
//   FloorDef.width, .height, .fields, .chutes, .teleporters; FloorField.flags;
//   FloorChute.x, .y, .depth; FloorTeleporter.x, .y, .x2, .y2, .z2; FLAG_ROCK.
// UNKNOWN: enumerator and member names of ArrivalOutcome, PortalOutcome,
//   DisplaceOutcome and LocateLine are assumed from the header comments.
// UNKNOWN: status1b.h helpers (pruneTheDead, setDead) have no signature in the
//   brief, so equivalent local helpers are used below instead of calling them.

#include "mordor/dungeon1b.h"

#include <cmath>

namespace mordor {
namespace {

// UNKNOWN: identifier of the Limited build is not given in the SRD half.
constexpr i32 kBuildLimited = 1;
constexpr double kChuteDamageStep = 10.5;   // R60 level * 10.5
constexpr i32 kWaterHurts = 1;               // R70 WaterHurts = 1 (scaling UNKNOWN)
constexpr i32 kIdentifyMin = 10;             // R72 clamp floor
constexpr i32 kIdentifyMax = 90;             // R72 clamp ceiling
constexpr i64 kFourSecondsMs = 4000;         // R73
// FIXED: renamed from kLocateLevelRange, which clashed with the header constant.
constexpr i32 kSoulSearchRange = 3;          // R88 LocateLevelRange

// FIXED: DisplaceOutcome enumerators are not declared in dungeon1b.h, so the
// outcome is carried by value. UNKNOWN: real DisplaceOutcome numbering.
constexpr i32 kDisplaceArrives = 0;
constexpr i32 kDisplaceFallsFromTheSky = 1;
constexpr i32 kDisplaceIntoRock = 2;

// FIXED: PortalOutcome enumerators are not declared in dungeon1b.h, so the
// outcome is carried by value. UNKNOWN: real PortalOutcome numbering.
constexpr i32 kPortalSurvived = 0;
constexpr i32 kPortalPartyDies = 1;

const FloorDef* floorDef(const GameData& data, i32 level) {
    if (level < 1 || level > static_cast<i32>(data.floors.size())) return nullptr;
    return &data.floors[static_cast<u32>(level - 1)];
}

// Square lookup: the field record of a 0-based square, or nullptr when off the map.
const FloorField* fieldAt(const GameData& data, i32 level, i32 x, i32 y) {
    const FloorDef* f = floorDef(data, level);
    if (f == nullptr) return nullptr;
    if (x < 0 || y < 0 || x >= f->width || y >= f->height) return nullptr;
    const i32 idx = y * f->width + x;
    if (static_cast<u32>(idx) >= f->fields.size()) return nullptr;
    return &f->fields[static_cast<u32>(idx)];
}

// Teleporter record standing on a 0-based square. Record coordinates are
// 1-based like chutes (UNKNOWN: teleporter coordinate convention assumed).
const FloorTeleporter* findTeleporter(const GameData& data, i32 level, i32 x, i32 y) {
    const FloorDef* f = floorDef(data, level);
    if (f == nullptr) return nullptr;
    for (u32 i = 0; i < f->teleporters.size(); ++i) {
        const FloorTeleporter& t = f->teleporters[i];
        if (t.x == x + 1 && t.y == y + 1) return &t;
    }
    return nullptr;
}

// UNKNOWN: dead-end level of the Limited build; taken as the clamp cap and deeper.
bool isLimitedDeadEnd(i32 build, i32 level) {
    return build == kBuildLimited && level >= kLimitedLevelCap;
}

// Writes the party position (0-based) and each member's location.
void setPartyPosition(Party& party, i32 x, i32 y, i32 level) {
    party.x = x;
    party.y = y;
    party.dungeonLevel = level;
    const u32 n = static_cast<u32>(party.size);
    for (u32 i = 0; i < n; ++i) {
        party.members[i].location.x = x;
        party.members[i].location.y = y;
        party.members[i].location.dungeonLevel = level;
    }
}

// Removes dead members from the party into the morgue list. The leader slot is
// kept. UNKNOWN: local equivalent of pruneTheDead (status1b.h).
void pruneDeadMembers(Party& party) {
    const u32 n = static_cast<u32>(party.size);
    if (n == 0) return;
    Arr<Character, kPartyMaxSize> kept{};
    kept[0] = party.members[0];
    u32 k = 1;
    for (u32 i = 1; i < n; ++i) {
        if (party.members[i].isDead()) {
            party.morgueList.push_back(party.members[i]);
        } else {
            kept[k++] = party.members[i];
        }
    }
    party.members = kept;
    party.size = static_cast<i32>(k);
}

// R62 / R78 saving: the member is written to the floor at the given level and
// square, into the occupants of the area of that square. The area of the square
// landed on is used (documented port fix).
// UNKNOWN: level 0 has no FloorState; members sent there are not saved.
void saveOnFloor(Vec<FloorState>& floors, const GameData& data, i32 level, i32 x, i32 y,
                 Character who) {
    if (level < 1 || level > static_cast<i32>(floors.size())) return;
    FloorState& fs = floors[static_cast<u32>(level - 1)];
    const i32 idx = squareIndexOf(data, level, x, y);
    i32 area = 0;
    if (idx >= 0 && static_cast<u32>(idx) < fs.squares.size()) {
        area = fs.squares[static_cast<u32>(idx)].areaId;
    }
    if (area < 0 || area >= 201) area = 0;
    who.location.x = x;
    who.location.y = y;
    who.location.dungeonLevel = level;
    who.location.area = area;
    fs.areas[static_cast<u32>(area)].occupants.push_back(who);
}

bool sameSquare(const TeleporterDest& a, const TeleporterDest& b) {
    return a.x == b.x && a.y == b.y && a.level == b.level;
}

// FIXED: DisplaceOutcome has no enumerators named FallsFromTheSky, IntoRock or
// Arrives in dungeon1b.h. Values are mapped through the local constants above.
DisplaceOutcome toDisplaceOutcome(ArrivalOutcome o) {
    switch (o) {
        case ArrivalOutcome::FallsFromTheSky:
            return static_cast<DisplaceOutcome>(kDisplaceFallsFromTheSky);
        case ArrivalOutcome::IntoRock:
            return static_cast<DisplaceOutcome>(kDisplaceIntoRock);
        default:
            return static_cast<DisplaceOutcome>(kDisplaceArrives);
    }
}

// FIXED: identifyChance(f64, bool) is not declared in dungeon1b.h, so the
// skill-based chance is computed by this local helper.
// UNKNOWN: the Chances formula is not in the SRD half. The skill is taken as a
// percentage, clamped to the R72 band, and darkness is ignored.
i32 identifyChanceFromSkill(f64 skill, bool darkness) {
    // R72: chance = skill + 20, clamped to 10..90 (VB Int = floor).
    // Clamp is applied before the darkness reduction, in the order R72 lists them.
    // SRD-AMBIGUOUS: R72 "darkness: chance costs a quarter" is read here as chance / 4
    // (the test and its comment assume this). A literal "costs a quarter" could mean chance * 3/4.
    i32 chance = static_cast<i32>(vbInt(skill + 20.0));
    if (chance < 10) {
        chance = 10;
    }
    if (chance > 90) {
        chance = 90;
    }
    if (darkness) {
        chance = chance / 4;  // integer division: VB Int of the quartered value
    }
    return chance;
}

}  // namespace

// ---------------------------------------------------------------------------
// Map helpers (R59, R75, R77, R83, R84, R86). Defined here because the chute
// and teleporter rules use them first.
// ---------------------------------------------------------------------------

i32 dungeonLevelCount(const GameData& data, i32 build) {
    (void)build;
    // R59 dungeon.Count. The limited build's denial happens in the R59 clamp and R67/R68.
    return static_cast<i32>(data.floors.size());
}

i32 mapWidth(const GameData& data, i32 level) {
    const FloorDef* f = floorDef(data, level);
    return f != nullptr ? f->width : 0;
}

i32 mapHeight(const GameData& data, i32 level) {
    const FloorDef* f = floorDef(data, level);
    return f != nullptr ? f->height : 0;
}

i32 squareIndexOf(const GameData& data, i32 level, i32 x, i32 y) {
    const FloorDef* f = floorDef(data, level);
    if (f == nullptr) return -1;
    if (x < 0 || y < 0 || x >= f->width || y >= f->height) return -1;
    return y * f->width + x;
}

bool isRockSquare(const GameData& data, i32 level, i32 x, i32 y) {
    // R75, R84 ROCK tile. UNKNOWN: ROCK is assumed to be FLAG_ROCK in FloorField.flags.
    const FloorField* fld = fieldAt(data, level, x, y);
    if (fld == nullptr) return false;
    return (static_cast<u32>(fld->flags) & (1u << static_cast<u32>(FLAG_ROCK))) != 0u;
}

bool isTeleporterSquare(const GameData& data, i32 level, i32 x, i32 y) {
    // R75 teleporter tile: a teleporter record stands on the square.
    return findTeleporter(data, level, x, y) != nullptr;
}

bool isUnmappedSquare(const GameData& data, i32 build, i32 level, i32 x, i32 y) {
    // UNKNOWN: exact test for unmapped. Taken as: the square is off the map
    // or the level has no floor data.
    (void)build;
    return squareIndexOf(data, level, x, y) < 0;
}

i32 limitedLevelClamp(i32 build, i32 level) {
    // R86 the Limited build clamps level to 3.
    return build == kBuildLimited ? minv(level, kLimitedLevelCap) : level;
}


// ---------------------------------------------------------------------------
// Walked and taken chutes (R56-R68)
// ---------------------------------------------------------------------------

i32 findChuteRecord(const GameData& data, i32 level, i32 x, i32 y) {
    // R56 first chute at (x+1, y+1) with Depth > 0; otherwise no fall.
    const FloorDef* f = floorDef(data, level);
    if (f == nullptr) return kNoRecord;
    for (u32 i = 0; i < f->chutes.size(); ++i) {
        const FloorChute& c = f->chutes[i];
        if (c.x == x + 1 && c.y == y + 1 && c.depth > 0) return static_cast<i32>(i);
    }
    return kNoRecord;
}

ChuteFloatRolls rollChuteFloats(Ctx& ctx, const Party& party) {
    // R57 one roll per member, leader first, even when following.
    // UNKNOWN: float chance is not in the SRD half. The draw is always made; a
    // levitating member floats.
    ChuteFloatRolls r{};
    const u32 n = static_cast<u32>(party.size);
    for (u32 i = 0; i < n; ++i) {
        ctx.rng.nextDouble();
        r.floats[i] = party.members[i].ailments.levitating;
    }
    return r;
}

ChuteFallPlan planChuteFall(const Party& party, const ChuteFloatRolls& rolls, bool followYes) {
    // R57, R58 pure plan from the rolls.
    ChuteFallPlan plan{};
    const u32 n = static_cast<u32>(party.size);
    if (n == 0) return plan;
    const bool leaderFloats = rolls.floats[0];
    plan.leaderFloatHint = leaderFloats;                 // R57 hint when the leader floats
    plan.askFollow = !leaderFloats && n >= 2;            // R58 Follow Leader box
    if (leaderFloats) {
        // R57 a floating leader goes nowhere, with the hint.
        // CONFLICT: R57 says "goes nowhere" but the original also reads as "the leader stays".
        // The SRD wording is followed.
        // UNKNOWN: whether non-floating members still fall when the leader floats. Kept up here.
        plan.goesNowhere[0] = true;
        for (u32 i = 1; i < n; ++i) {
            if (rolls.floats[i]) plan.goesNowhere[i] = true;  // R57 floaters that do not follow
        }
        return plan;
    }
    plan.falls[0] = true;
    for (u32 i = 1; i < n; ++i) {
        if (followYes) {
            plan.falls[i] = true;                        // R58 Yes: sent down, no roll
        } else if (rolls.floats[i]) {
            plan.goesNowhere[i] = true;                  // R57 floated and did not follow
        }
    }
    return plan;
}

i32 chuteDestinationLevel(const GameData& data, i32 build, i32 fromLevel, i32 depth) {
    // R59 to = clamp(from + max(0, depth), 1, dungeon.Count)
    const i32 count = dungeonLevelCount(data, build);
    return clampv(fromLevel + maxv(0, depth), 1, count);
}

i32 rollChuteDamage(Ctx& ctx, i32 level, bool levitating) {
    // R60 damage = RoundHalfEven(Floor(Rnd * level * 10.5) * Log(4) + 5); 0 if levitating.
    if (levitating) return 0;
    const double raw = static_cast<double>(vbInt(ctx.rng.nextDouble() * (level * kChuteDamageStep)))
                       * std::log(4.0) + 5.0;
    return static_cast<i32>(roundHalfEven(raw));
}

void reportChuteOnScreen(Ctx& ctx, const Party& party, const ChuteFallPlan& plan) {
    // R61 members who did not fall, if on screen, get the chute message (once).
    bool shown = false;
    const u32 n = static_cast<u32>(party.size);
    for (u32 i = 0; i < n; ++i) {
        if (!plan.falls[i] && party.members[i].visible) shown = true;
    }
    if (shown) {
        ctx.say("chute.here", "There is a chute here.  Select TAKE to use.", {}, {});
    }
}

void chuteBreakUp(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                  i32 belowLevel, i32 landX, i32 landY) {
    // R62 sub0908 break-up. Anyone off the leader's level has disappeared from view.
    // A member at hits <= 0 dies there. Others are saved on the floor below at the
    // landing square and dropped from the party.
    const u32 n = static_cast<u32>(party.size);
    if (n == 0) return;
    const i32 leaderLevel = party.members[0].location.dungeonLevel;
    Arr<Character, kPartyMaxSize> kept{};
    kept[0] = party.members[0];
    u32 k = 1;
    for (u32 i = 1; i < n; ++i) {
        Character who = party.members[i];
        if (who.location.dungeonLevel == leaderLevel) {
            kept[k++] = who;
            continue;
        }
        ctx.say("chute.disappear", "has disappeared from view!", {}, {who.name});
        if (who.vitals.curHits <= 0) {
            // UNKNOWN: SetDead semantics; direct death state used.
            who.deathState = DeathState::Dead;
        }
        saveOnFloor(floors, data, belowLevel, landX, landY, who);
    }
    party.members = kept;
    party.size = static_cast<i32>(k);
}

void afterAFall(Ctx& ctx, Party& party) {
    // R63 a lone character at CurHits <= 0 and not dead gets Kill("").
    (void)ctx;
    if (party.size != 1) return;
    Character& who = party.members[0];
    if (who.vitals.curHits > 0 || who.isDead()) return;
    // UNKNOWN: Kill(_ch, "") cause text is empty; direct death state used.
    who.deathState = DeathState::Dead;
}

void changeLevel(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                 i32& flags, i32 newLevel) {
    // R64 ChangeLevel.
    // UNKNOWN: the full ChangeLevel body is not in this SRD half. It sets the
    // new level and then settles the area.
    party.dungeonLevel = newLevel;
    const u32 n = static_cast<u32>(party.size);
    for (u32 i = 0; i < n; ++i) party.members[i].location.dungeonLevel = newLevel;
    settleArea(ctx, party, floors, data, flags);
}

void settleAfterChute(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                      i32& flags, bool leaderChangedLevel, bool chuteUnderCaster) {
    // R64 if the leader changed floor, ChangeLevel runs. Otherwise the square
    // settles, unless a chute under the caster (the fall settles the area).
    if (leaderChangedLevel) {
        changeLevel(ctx, party, floors, data, flags, party.dungeonLevel);
        return;
    }
    if (!chuteUnderCaster) settleArea(ctx, party, floors, data, flags);
}

bool chuteRerunsSquare(const Party& party, bool takenByTake) {
    // R65 TAKE always re-runs. A walked chute re-runs when a companion is with the party.
    // CONFLICT: the count of re-runs is not settled by the dossier; one re-run is used.
    if (takenByTake) return true;
    return party.aliveCompanions > 0;
}

void partyListUpdate(Ctx& ctx, Party& party, Vec<Companion>& companions) {
    // R66 the party-list UPDATE runs after walked chutes and pit falls. It prunes the
    // dead. Companions of a dropped member are not released here (sub0930 does that).
    (void)ctx;
    (void)companions;
    pruneDeadMembers(party);
}

void rerunSquare(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                 i32& flags) {
    // R65, R81 re-run of the square. CONFLICT: re-run count. The port only settles and
    // describes the new square (quirk note), so a chute or teleporter here does not fire
    // again and no loop guard is needed.
    settleArea(ctx, party, floors, data, flags);
}

ChuteWalkResult walkChute(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                          i32 build, Vec<Companion>& companions, i32& flags, bool followYes) {
    // R56-R66 orchestrated walked chute.
    ChuteWalkResult res{};
    const i32 level = party.dungeonLevel;
    const i32 rec = findChuteRecord(data, level, party.x, party.y);   // R56
    if (rec == kNoRecord) return res;                                 // R56 no fall
    res.recordFound = true;
    const FloorChute& chute = floorDef(data, level)->chutes[static_cast<u32>(rec)];
    res.destLevel = chuteDestinationLevel(data, build, level, chute.depth);   // R59
    if (isLimitedDeadEnd(build, level)) {                              // R68
        reportLimitedChuteWalkOn(ctx);
        return res;
    }
    const u32 n = static_cast<u32>(party.size);
    const ChuteFloatRolls rolls = rollChuteFloats(ctx, party);         // R57
    const ChuteFallPlan plan = planChuteFall(party, rolls, followYes); // R57, R58
    if (plan.leaderFloatHint) {
        // UNKNOWN: text of the leader's float hint is not in the SRD half.
        ctx.say("chute.floatHint", "You float over the chute.", {}, {});
    }
    reportChuteOnScreen(ctx, party, plan);                             // R61
    if (plan.falls[0]) {
        res.fell = true;
        res.leaderChangedLevel = true;
        for (u32 i = 0; i < n; ++i) {
            if (!plan.falls[i]) continue;
            Character& who = party.members[i];
            who.location.dungeonLevel = res.destLevel;
            const i32 dmg = rollChuteDamage(ctx, res.destLevel, who.ailments.levitating);  // R60
            who.vitals.curHits -= dmg;
            if (dmg > 0) {
                // Text carries N; the value is in nums.
                ctx.say("chute.fell", "You have fallen into a chute for N hits.",
                        {static_cast<i64>(dmg)}, {});
            }
            if (i == 0) res.damage = dmg;
        }
        party.dungeonLevel = res.destLevel;
        chuteBreakUp(ctx, party, floors, data, res.destLevel, party.x, party.y);   // R62
        afterAFall(ctx, party);                                                    // R63
        settleAfterChute(ctx, party, floors, data, flags, true, false);            // R64
    }
    res.rerun = chuteRerunsSquare(party, false);                                   // R65
    partyListUpdate(ctx, party, companions);                                       // R66
    if (res.rerun) rerunSquare(ctx, party, floors, data, flags);                   // R65
    return res;
}

ChuteTakeOutcome takeChute(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                           i32 build, Vec<Companion>& companions, i32& flags, i32 recordIndex) {
    // R67 TAKE sends everyone down with no roll, no question and no sound.
    (void)companions;
    const i32 level = party.dungeonLevel;
    if (isLimitedDeadEnd(build, level)) {                              // R67 refused
        ctx.say("chute.takeRefused", "You cannot take this Chute in the Limited Version.", {}, {});
        return ChuteTakeOutcome::RefusedLimitedDeadEnd;
    }
    i32 depth = 0;
    const FloorDef* f = floorDef(data, level);
    if (f != nullptr && recordIndex >= 0 && recordIndex < static_cast<i32>(f->chutes.size())) {
        depth = f->chutes[static_cast<u32>(recordIndex)].depth;
    }
    const i32 destLevel = chuteDestinationLevel(data, build, level, depth);   // R59
    changeLevel(ctx, party, floors, data, flags, destLevel);                   // R64 ChangeLevel
    if (chuteRerunsSquare(party, true)) {                                      // R65 TAKE always
        rerunSquare(ctx, party, floors, data, flags);
    }
    return ChuteTakeOutcome::Dropped;
}

void reportLimitedChuteWalkOn(Ctx& ctx) {
    // R68 walk-on on the limited dead-end level.
    ctx.say("chute.limitedWalk",
            "There is a chute here, but it doesn't do anything in the Limited Version.", {}, {});
}

// ---------------------------------------------------------------------------
// Stairs, water and ticks (R69-R73)
// ---------------------------------------------------------------------------

bool canClimbUp(const Character& who) {
    // R69 up-climb refused when paralysed or in the death state.
    return !who.ailments.paralysed && !who.isDead();
}

StairsOutcome useStairs(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                        i32 build, i32& flags, bool up) {
    // R69 UseStairs. Stairs down and chutes sit outside the paralysis test.
    if (up && !canClimbUp(party.members[0])) {
        return party.members[0].ailments.paralysed ? StairsOutcome::RefusedParalysed
                                                   : StairsOutcome::RefusedDead;
    }
    const i32 count = dungeonLevelCount(data, build);
    // UNKNOWN: landing square of the stairs is not in the SRD half; only the level changes.
    const i32 newLevel = clampv(party.dungeonLevel + (up ? -1 : 1), 0, count);
    changeLevel(ctx, party, floors, data, flags, newLevel);
    return StairsOutcome::Taken;
}

bool rollWaterItemLoss(Ctx& ctx, i32 dungeonLevel) {
    // R71 takes if rng.Next(100) < dungeonLevel/25.0*100.
    return static_cast<double>(ctx.rng.below(100)) < dungeonLevel / 25.0 * 100.0;
}

void applyWaterTick(Ctx& ctx, Character& who, Inventory& inventory, i32 dungeonLevel,
                    bool quicksand, bool levitating) {
    // R70 water and quicksand cost hits each tick unless levitating.
    if (levitating) return;
    // RESOLVED-BY-REFERENCE: scaling. Rules.cs WaterHurts doc (FRMDUNM.BAS:500-525): "costs you hits equal to the dungeon level, every tick".
    who.vitals.curHits -= static_cast<i32>(kWaterHurts) * dungeonLevel;
    // R70 quicksand may take a pack slot, using the R71 roll.
    // CONFLICT: R71 names water and quicksand; the roll is applied to quicksand only.
    if (quicksand && rollWaterItemLoss(ctx, dungeonLevel)) {
        const u32 slot = static_cast<u32>(ctx.rng.below(kPackSlots));
        if (inventory.pack[slot].id != 0) inventory.pack[slot] = Item{};
    }
}

i32 identifyChance(const Character& who, bool darkness, bool fog) {
    // Formulas, Identify skill: skill = ((INT+WIS)/2) * Ln((topGuildLevel+5)/10 + 1), 0 if topGuildLevel <= 0.
    // Then the Chances formula (see identifyChanceFromSkill).
    // UNKNOWN: the fog argument has no SRD effect on identify chance; accepted and ignored.
    (void)fog;
    f64 skill = 0.0;
    if (who.topGuildLevel > 0) {
        const f64 intWis =
            (who.abilities.stats[static_cast<u32>(StatIndex::Int)] +
             who.abilities.stats[static_cast<u32>(StatIndex::Wis)]) / 2.0;
        skill = intWis * std::log((who.topGuildLevel + 5) / 10.0 + 1.0);
    }
    // FIXED: call the skill-based helper; identifyChance(f64, bool) is not a declared overload.
    return identifyChanceFromSkill(skill, darkness);
}

bool rollIdentify(Ctx& ctx, const Character& who, bool darkness, bool fog) {
    // R72 rolled per group, per character, per tick.
    return ctx.rng.percent(identifyChance(who, darkness, fog));
}

bool fourSecondTickDue(i64 nowMs, i64 lastTickMs) {
    // R73 true when four seconds have passed.
    return nowMs - lastTickMs >= kFourSecondsMs;
}

void runFourSecondTick(Ctx& ctx, Party& party, Inventory& inventory,
                       i64 nowMs, i64& lastTickMs) {
    // R73 every four seconds the ailment and item tick runs.
    if (!fourSecondTickDue(nowMs, lastTickMs)) return;
    lastTickMs = nowMs;
    // UNKNOWN: the body of the ailment and item tick is not in this SRD half. Only the
    // clock advance is modelled.
    (void)ctx;
    (void)party;
    (void)inventory;
}

// ---------------------------------------------------------------------------
// Teleporter squares (R74-R82)
// ---------------------------------------------------------------------------

i32 teleporterLevel(i32 z2, i32 fromLevel) {
    // R76 Z2 if Z2 > 0 and differs from the current level; otherwise the current level.
    return (z2 > 0 && z2 != fromLevel) ? z2 : fromLevel;
}

i32 teleporterLoadedLevel(const Party& party, const GameData& data, i32 build) {
    // R75 loaded = leader.level if 1 <= leader.level <= dungeon.Count.
    // UNKNOWN: leader.level is read as location.dungeonLevel. Fallback when out of range
    // is the party level (not in the SRD half).
    const i32 leaderLevel = party.members[0].location.dungeonLevel;
    const i32 count = dungeonLevelCount(data, build);
    if (leaderLevel >= 1 && leaderLevel <= count) return leaderLevel;
    return party.dungeonLevel;
}

Vec<TeleporterDest> rollTeleporterDestinations(Ctx& ctx, const GameData& data, i32 build,
                                               i32 fromLevel, const FloorTeleporter& tp,
                                               i32 memberCount) {
    // R75 X2/Y2/Z2 fixed or random. R76 level and loaded level.
    // RESOLVED-BY-REFERENCE: Teleporting.cs:35 (ForSquare) starts with "int loaded = fromLevel;"
    // and draws each random square on "dungeon[loaded - 1]". After each member it sets
    // "if (dest[0].Level >= 1 && dest[0].Level <= dungeon.Count) loaded = dest[0].Level;".
    // So the leader's draw is on fromLevel, and every later member's draw is on the level the
    // leader went to (when that is in range). The stored level is still the destination level.
    Vec<TeleporterDest> dests;
    const i32 count = dungeonLevelCount(data, build);
    const i32 level = teleporterLevel(tp.z2, fromLevel);       // R76
    const bool randomDest = (tp.x2 == 0 || tp.y2 == 0);
    i32 loaded = fromLevel;
    for (i32 m = 0; m < memberCount; ++m) {
        TeleporterDest d{};
        d.level = level;
        if (randomDest) {
            // R75 each member drawn fresh on the loaded level, redrawn until neither rock nor teleporter.
            const i32 w = mapWidth(data, loaded);
            const i32 h = mapHeight(data, loaded);
            do {
                d.x = w > 0 ? ctx.rng.below(w) : 0;
                d.y = h > 0 ? ctx.rng.below(h) : 0;
            } while (w > 0 && h > 0 &&
                     (isRockSquare(data, loaded, d.x, d.y) || isTeleporterSquare(data, loaded, d.x, d.y)));
        } else {
            d.x = tp.x2 - 1;                                   // R75 fixed, 1-based stored 0-based
            d.y = tp.y2 - 1;
        }
        d.unmapped = isUnmappedSquare(data, build, level, d.x, d.y);   // R77
        dests.push_back(d);
        // Reference: loaded follows the leader's destination once that level is in range.
        if (level >= 1 && level <= count) loaded = level;
    }
    return dests;
}

// Keeper becomeLost(Ctx&, LostState&) lives in src/dungeon1a.cpp.
void becomeLost(Ctx& ctx, LostState& lost);

void becomeLost(Ctx& ctx, Character& who) {
    // R77 BecomeLost sets the three lost flags; delegates to the keeper.
    becomeLost(ctx, who.lost);
}

void splitTeleportedParty(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                          Vec<Companion>& companions, const Vec<TeleporterDest>& dests) {
    // R78 members i >= 1 with dest[i] != dest[0] leave the party and are saved where they landed.
    // R79 documents that random teleporters almost always split the party.
    // UNKNOWN: companion release on split is not in the SRD half.
    (void)ctx;
    (void)companions;
    const u32 n = static_cast<u32>(party.size);
    if (n == 0 || dests.size() == 0) return;
    const TeleporterDest d0 = dests[0];
    Arr<Character, kPartyMaxSize> kept{};
    kept[0] = party.members[0];
    u32 k = 1;
    for (u32 i = 1; i < n; ++i) {
        const TeleporterDest di = (i < dests.size()) ? dests[i] : d0;
        Character who = party.members[i];
        if (sameSquare(di, d0)) {
            kept[k++] = who;
            continue;
        }
        if (who.vitals.curHits <= 0) {                 // R78
            who.vitals.curHits = 0;
            // UNKNOWN: SetDead semantics; direct death state used.
            who.deathState = DeathState::Dead;
        }
        saveOnFloor(floors, data, di.level, di.x, di.y, who);
    }
    party.members = kept;
    party.size = static_cast<i32>(k);
}

void finishTeleportLeader(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                          i32& flags, const TeleporterDest& leaderDest, bool partyShrank) {
    // R80 if the party shrank: PruneTheDead. The leader moves to dest[0].
    // AfterAFall, RerunSquare (no loop guard), SettleArea last.
    if (partyShrank) pruneDeadMembers(party);                  // R80
    const i32 oldLevel = party.dungeonLevel;
    setPartyPosition(party, leaderDest.x, leaderDest.y, oldLevel);   // R80 leader to dest[0]
    if (leaderDest.level != oldLevel) {
        // UNKNOWN: level 0 is not checked on the teleporter path (quirk note R-level 0).
        changeLevel(ctx, party, floors, data, flags, leaderDest.level);
    }
    afterAFall(ctx, party);                                    // R80
    rerunSquare(ctx, party, floors, data, flags);              // R80, R81
    settleArea(ctx, party, floors, data, flags);               // R80 last
}

void walkOntoTeleporter(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                        i32 build, Vec<Companion>& companions,
                        i32& flags, const FloorTeleporter& tp) {
    // R74 walking on a teleporter: no fall and no rock death.
    const i32 from = party.dungeonLevel;
    const i32 destLevel = teleporterLevel(tp.z2, from);        // R76
    const i32 lastLevel = dungeonLevelCount(data, build);
    if (destLevel > lastLevel) {                                // R82 only logs
        reportTeleportPastLastLevel(ctx, destLevel, lastLevel);
        return;
    }
    const Vec<TeleporterDest> dests = rollTeleporterDestinations(ctx, data, build, from, tp,
                                                                 party.size);   // R75
    if (dests.size() == 0) return;
    const u32 n = static_cast<u32>(party.size);
    for (u32 i = 0; i < n && i < dests.size(); ++i) {
        if (dests[i].unmapped) becomeLost(ctx, party.members[i]);              // R77
    }
    const i32 before = party.size;
    splitTeleportedParty(ctx, party, floors, data, companions, dests);        // R78, R79
    const bool shrank = party.size < before;
    finishTeleportLeader(ctx, party, floors, data, flags, dests[0], shrank);  // R80
}

void reportTeleportPastLastLevel(Ctx& ctx, i32 destLevel, i32 lastLevel) {
    // R82 a record past the last level only logs a message.
    // UNKNOWN: message text is not in the SRD half.
    ctx.say("teleport.pastLast", "The teleporter leads past the last level.",
            {static_cast<i64>(destLevel), static_cast<i64>(lastLevel)}, {});
}

// ---------------------------------------------------------------------------
// Teleport, displacement, Ethereal Portal and Soul Search (R83-R88)
// ---------------------------------------------------------------------------

Location displaceTarget(const Location& from, Facing facing, i32 right, i32 forward, i32 down) {
    // R83 destination relative to facing, 1-based for the formula, wrapped 30x30.
    // Location is stored 0-based.
    i32 nx = from.x + 1;
    i32 ny = from.y + 1;
    switch (facing) {
        case Facing::N: nx += right;   ny += forward; break;
        case Facing::E: nx += forward; ny -= right;   break;
        case Facing::S: nx -= right;   ny -= forward; break;
        case Facing::W: nx -= forward; ny += right;   break;
    }
    nx = wrapCoordinate(nx, kTeleportWrapSize);
    ny = wrapCoordinate(ny, kTeleportWrapSize);
    i32 level = from.dungeonLevel;
    if (down != 0) level += clampv(down, -kDisplaceLevelLimit, kDisplaceLevelLimit);
    if (level < 0) level = 0;
    Location out{};
    out.x = nx - 1;
    out.y = ny - 1;
    out.dungeonLevel = level;
    out.facing = from.facing;
    return out;
}

ArrivalOutcome teleportArrivalTest(const GameData& data, i32 build, i32 level, i32 x, i32 y) {
    // R84 ForSpell order: FallsFromTheSky, IntoRock, Arrives. R86 clamp applies first.
    // UNKNOWN: MaxDungeonLevel (mc0276) value is not in the SRD half; dungeon.Count is used.
    const i32 lvl = limitedLevelClamp(build, level);           // R86
    if (lvl <= 0) return ArrivalOutcome::FallsFromTheSky;      // R84(a)
    if (lvl > dungeonLevelCount(data, build)) return ArrivalOutcome::IntoRock;   // R84(b)
    if (squareIndexOf(data, lvl, x, y) < 0) return ArrivalOutcome::IntoRock;    // R84(c)
    if (isRockSquare(data, lvl, x, y)) return ArrivalOutcome::IntoRock;         // R84(d)
    return ArrivalOutcome::Arrives;                                              // R84(e)
}

// FIXED: forward declaration, applyFailedTeleport is used by displaceParty and
// etherealPortalStep before its definition below. Removed `static` so the
// declaration matches the later extern-linkage definition (namespace mordor, not
// an anonymous namespace).
void applyFailedTeleport(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                         i32 build, ArrivalOutcome outcome,
                         i32 level, i32 x, i32 y);

DisplaceOutcome displaceParty(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                              i32 build, i32& flags,
                              i32 right, i32 forward, i32 down) {
    // R83-R85 Teleport spell move.
    Location from{};
    from.x = party.x;
    from.y = party.y;
    from.dungeonLevel = party.dungeonLevel;
    from.facing = static_cast<i32>(party.facing);
    const Location target = displaceTarget(from, party.facing, right, forward, down);   // R83
    const i32 level = limitedLevelClamp(build, target.dungeonLevel);                     // R86
    const ArrivalOutcome outcome = teleportArrivalTest(data, build, level, target.x, target.y);  // R84
    if (outcome != ArrivalOutcome::Arrives) {
        applyFailedTeleport(ctx, party, floors, data, build, outcome, level, target.x, target.y);  // R85
        return toDisplaceOutcome(outcome);
    }
    const i32 oldLevel = party.dungeonLevel;
    setPartyPosition(party, target.x, target.y, oldLevel);
    if (level != oldLevel) {
        changeLevel(ctx, party, floors, data, flags, level);
    } else {
        settleArea(ctx, party, floors, data, flags);
    }
    // FIXED: DisplaceOutcome::Arrives is not declared; value supplied by constant.
    return static_cast<DisplaceOutcome>(kDisplaceArrives);
}

void applyFailedTeleport(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                         i32 build, ArrivalOutcome outcome,
                         i32 level, i32 x, i32 y) {
    // R85 failed teleport.
    // Level 0: the party falls and is killed.
    // Rock, or below the deepest level: the square is written as rock on the automap, and
    // everyone is killed with a cause that only Resurrect undoes.
    // UNKNOWN: the cause enum for rock death is not in the SRD half. DeathState::Rock is used.
    (void)ctx;
    (void)build;
    if (outcome == ArrivalOutcome::Arrives) return;
    const u32 n = static_cast<u32>(party.size);
    if (outcome == ArrivalOutcome::FallsFromTheSky) {
        for (u32 i = 0; i < n; ++i) party.members[i].deathState = DeathState::Dead;
        return;
    }
    if (level >= 1 && level <= static_cast<i32>(floors.size())) {
        FloorState& fs = floors[static_cast<u32>(level - 1)];
        const i32 idx = squareIndexOf(data, level, x, y);
        if (idx >= 0 && static_cast<u32>(idx) < fs.squares.size()) {
            // UNKNOWN: automap field; rockRecorded marks the square as rock on the automap.
            fs.squares[static_cast<u32>(idx)].rockRecorded = true;
        }
    }
    for (u32 i = 0; i < n; ++i) party.members[i].deathState = DeathState::Rock;
}

PortalOutcome etherealPortalStep(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                                 i32 build, i32& flags) {
    // R87 StepThrough: one square forward, wrapping at edges. The party moves first,
    // then the rock test runs. True means the party dies.
    // CONFLICT: which square is tested. The new square is used.
    Location from{};
    from.x = party.x;
    from.y = party.y;
    from.dungeonLevel = party.dungeonLevel;
    from.facing = static_cast<i32>(party.facing);
    const Location next = displaceTarget(from, party.facing, 0, 1, 0);
    setPartyPosition(party, next.x, next.y, party.dungeonLevel);       // party moves first
    if (isRockSquare(data, party.dungeonLevel, next.x, next.y)) {      // rock test
        applyFailedTeleport(ctx, party, floors, data, build, ArrivalOutcome::IntoRock,
                            party.dungeonLevel, next.x, next.y);
        // FIXED: PortalOutcome::PartyDies is not declared; value supplied by constant.
        return static_cast<PortalOutcome>(kPortalPartyDies);
    }
    settleArea(ctx, party, floors, data, flags);                       // UNKNOWN: settle after the step
    // FIXED: PortalOutcome::Survived is not declared; value supplied by constant.
    return static_cast<PortalOutcome>(kPortalSurvived);
}

// R88 Soul Search direction: maps the signs of the right and forward offsets to one of
// the eight RelativeDirection points. UNKNOWN: the point assignment is assumed from the
// header order (North first, then clockwise).
namespace {
RelativeDirection soulSearchDirection(i32 right, i32 forward) {
    const i32 sr = (right > 0) - (right < 0);
    const i32 sf = (forward > 0) - (forward < 0);
    if (sf > 0 && sr == 0) return RelativeDirection::North;
    if (sf > 0 && sr > 0) return RelativeDirection::NorthEast;
    if (sf == 0 && sr > 0) return RelativeDirection::East;
    if (sf < 0 && sr > 0) return RelativeDirection::SouthEast;
    if (sf < 0 && sr == 0) return RelativeDirection::South;
    if (sf < 0 && sr < 0) return RelativeDirection::SouthWest;
    if (sf == 0 && sr < 0) return RelativeDirection::West;
    if (sf > 0 && sr < 0) return RelativeDirection::NorthWest;
    return RelativeDirection::North;
}
}  // namespace

}  // namespace mordor

namespace mordor {

bool export_isRockSquare(const GameData& data, i32 level, i32 x, i32 y) {
    return isRockSquare(data, level, x, y);
}

}  // namespace mordor
