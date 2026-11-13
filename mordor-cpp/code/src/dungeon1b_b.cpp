// dungeon1b_b.cpp : rules R89-R122 (forced settle, area change, rotator, roster,
// bodies and carrying, rescue, chests, traps, ask to join).
// Cross-module calls whose signatures are not in the API INDEX are omitted and marked UNKNOWN.
#include <cmath>

#include "mordor/dungeon1b.h"
#include "mordor/death1a.h"  // FIXED: AskToJoinOutcome is declared in death1a.h, not dungeon1b.h

namespace mordor {
namespace {

// R89 forced flag bit and source ids. UNKNOWN: exact values of gv1D6A bit and ForcedSettleSource not in API INDEX.
constexpr i32 kForcedSettleBit = 1;
constexpr i32 kForceTeleporterSquare = 1;
constexpr i32 kForceTeleportSpell = 2;
constexpr i32 kForceChestTeleporter = 3;
constexpr i32 kAreaCount = 201;          // FloorState::areas size
constexpr i32 kRescueDayCap = 2000000;   // R114 loop cap

Str intStr(i64 v) {
    Str out;
    if (v < 0) {
        out += '-';
        v = -v;
    }
    char buf[24];
    i32 n = 0;
    do {
        buf[n++] = static_cast<char>('0' + (v % 10));
        v /= 10;
    } while (v > 0);
    while (n > 0) {
        out += buf[--n];
    }
    return out;
}

i32 floorIndex(const Vec<FloorState>& floors, i32 level) {
    for (u32 i = 0; i < static_cast<u32>(floors.size()); ++i) {
        if (floors[i].dungeonLevel == level) {
            return static_cast<i32>(i);
        }
    }
    return -1;
}

i32 currentArea(const Party& party) {
    return party.members[0].location.area;
}

// Index of the member carrying a dead body, -1 if none.
i32 carrierIndex(const Party& party) {
    if (party.carryingSlot != CarryingSlot::DeadCharacter) {
        return -1;
    }
    for (i32 i = 0; i < party.size; ++i) {
        if (!party.members[static_cast<u32>(i)].carryingCharacterName.empty()) {
            return i;
        }
    }
    return -1;
}

// UNKNOWN: carried body is identified by the carrier's carryingCharacterName equal to the body name.
bool isCarriedBody(const Party& party, i32 idx) {
    if (party.carryingSlot != CarryingSlot::DeadCharacter) {
        return false;
    }
    const Str name = party.members[static_cast<u32>(idx)].name;
    if (name.empty()) {
        return false;
    }
    for (i32 j = 0; j < party.size; ++j) {
        if (party.members[static_cast<u32>(j)].carryingCharacterName == name) {
            return true;
        }
    }
    return false;
}

// Removes member idx from the party, shifting the rest left (UNKNOWN: representation of party removal).
void removeMember(Party& party, i32 idx) {
    removePartyMember(party, idx);
}

void setPartyPosition(Party& party, i32 level, i32 x, i32 y) {
    party.dungeonLevel = level;
    party.x = x;
    party.y = y;
    for (i32 i = 0; i < party.size; ++i) {
        Location& loc = party.members[static_cast<u32>(i)].location;
        loc.dungeonLevel = level;
        loc.x = x;
        loc.y = y;
    }
}

const char* facingText(Facing f) {
    // R101 readout: the original prints the facing's full name (LookAround.cs:127, facing.ToString()).
    switch (f) {
        case Facing::N: return "North";
        case Facing::E: return "East";
        case Facing::S: return "South";
        case Facing::W: return "West";
        default: return "West";  // UNKNOWN: out-of-range facing; W is the original switch's default
    }
}

// R106 / R104: put down a carried dead body. UNKNOWN: companion release needs the companion list (handled in putDownWhatIsCarried only).
void putDownBodyOnly(Ctx& ctx, Party& party) {
    if (party.carryingSlot != CarryingSlot::DeadCharacter) {
        return;
    }
    const i32 c = carrierIndex(party);
    if (c >= 0) {
        party.members[static_cast<u32>(c)].carryingCharacterName = Str();
    }
    party.carryingSlot = CarryingSlot::None;
    ctx.say("dungeon.putDown", "You put down the body.", {}, {});
}

}  // namespace

// ---------------------------------------------------------------- R89-R95 area change and forced settle

// R89 only the three listed sources set the forced flag.
// CONFLICT: c30 also lists level squares and the editor; only the three SRD sources are implemented.
void markForcedSettle(i32& flags, i32 source) {
    if (source == kForceTeleporterSquare || source == kForceTeleportSpell || source == kForceChestTeleporter) {
        flags |= kForcedSettleBit;
    }
}

// R90 takes and clears the flag. R93 same area and not forced returns false.
bool settleArea(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data, i32& flags) {
    const bool forced = (flags & kForcedSettleBit) != 0;  // R90 take
    flags &= ~kForcedSettleBit;                           // R90 clear
    const i32 fi = floorIndex(floors, party.dungeonLevel);
    if (fi < 0) {
        return false;  // UNKNOWN: no floor record for this level
    }
    FloorState& floor = floors[static_cast<u32>(fi)];
    const i32 area = currentArea(party);
    // UNKNOWN: "same area" is tested against FloorState::roomStamp, the loaded area id (-1 = none loaded).
    if (floor.roomStamp != static_cast<i64>(area)) {
        changeAreaDifferent(ctx, party, floors, data, flags, area);  // R94 different area
        return true;
    }
    if (!forced) {
        return false;  // R93
    }
    settleForcedSameArea(ctx, party, floors, data, flags);  // R91
    return true;
}

// R90 AfterTurn: a flag still set calls SettleArea.
void afterTurn(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data, i32& flags) {
    if ((flags & kForcedSettleBit) != 0) {
        settleArea(ctx, party, floors, data, flags);
    }
}

// R91 same area and forced; R92 timCombat off.
void settleForcedSameArea(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data, i32& flags) {
    (void)flags;  // flag already taken by settleArea (R90)
    const i32 fi = floorIndex(floors, party.dungeonLevel);
    if (fi < 0) {
        return;
    }
    FloorState& floor = floors[static_cast<u32>(fi)];
    const i32 area = currentArea(party);
    // (a) R91 room stored. UNKNOWN: write-back target reduced to the loaded-area id.
    floor.roomStamp = static_cast<i64>(area);
    // (b) R91 engaged group takes a free round (MonstersRound AttackKind 2). UNKNOWN: MonstersRound not in API INDEX; not called.
    // (c) R91 character saved.
    saveCharacters(ctx, party, floors, data);
    // (d) R91 carried bodies put down. Companion release needs the companion list, so only bodies are handled here.
    putDownBodyOnly(ctx, party);
    // (e) R91 nearby others reported.
    reportLookAround(ctx, party, floor);
    // R92 timCombat off, no sub0A91 reload. UNKNOWN: timCombat mapped to Party::engaged.
    party.engaged = false;
    // (f) R91 Arrived() runs.
    arrived(ctx, party, floors, data);
}

// R94 LeaveArea. UNKNOWN: hostile groups not fighting get a leave round (MonstersRound not in API INDEX; not called).
void leaveArea(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data) {
    (void)ctx;
    (void)party;
    (void)data;
    const i32 fi = floorIndex(floors, party.dungeonLevel);
    if (fi < 0) {
        return;
    }
    // The old room is no longer loaded.
    floors[static_cast<u32>(fi)].roomStamp = -1;
}

// R94 LoadArea. UNKNOWN: room contents load (sub0A91) and arrival round are not in API INDEX; loaded area id recorded only.
void loadArea(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data, bool arrivalRound) {
    (void)ctx;
    (void)data;
    (void)arrivalRound;  // arrival round not called (MonstersRound not in API INDEX)
    const i32 fi = floorIndex(floors, party.dungeonLevel);
    if (fi < 0) {
        return;
    }
    floors[static_cast<u32>(fi)].roomStamp = static_cast<i64>(currentArea(party));
}

// R94 SaveChar. UNKNOWN: storage representation; each member is marked saved.
void saveCharacters(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data) {
    (void)ctx;
    (void)floors;
    (void)data;
    for (i32 i = 0; i < party.size; ++i) {
        party.members[static_cast<u32>(i)].saved = true;
    }
}

// R91, R94 Arrived(): the look-around report of R102. UNKNOWN: any further arrival text not in SRD.
void arrived(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data) {
    (void)data;
    const i32 fi = floorIndex(floors, party.dungeonLevel);
    if (fi < 0) {
        return;
    }
    reportLookAround(ctx, party, floors[static_cast<u32>(fi)]);
}

// R94 different area path: LeaveArea; PruneTheDead; SaveChar; Arrived; LoadArea(arrivalRound: true).
// UNKNOWN: PruneTheDead signature not in API INDEX, so the call is omitted.
void changeAreaDifferent(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data,
                         i32& flags, i32 newArea) {
    (void)flags;
    leaveArea(ctx, party, floors, data);
    // UNKNOWN: PruneTheDead() not called (signature not in API INDEX).
    saveCharacters(ctx, party, floors, data);
    for (i32 i = 0; i < party.size; ++i) {
        party.members[static_cast<u32>(i)].location.area = newArea;
    }
    arrived(ctx, party, floors, data);
    loadArea(ctx, party, floors, data, true);
}

// ---------------------------------------------------------------- R96-R98 rotator, lead group, Asmodeus

// R96 random facing. Port uses rng.Next(4) as 0..3.
Facing rollRotatorFacing(Ctx& ctx) {
    return static_cast<Facing>(ctx.rng.below(4));
}

// R97 LeadGroup: combat has monsters -> monsters[0] if any alive, else null;
// no monsters -> waiting[0] or null.
const Creature* leadGroupCreature(const Encounter& combat, const Vec<Creature>& waiting) {
    if (!combat.monsters.empty()) {
        for (u32 i = 0; i < static_cast<u32>(combat.monsters.size()); ++i) {
            if (combat.monsters[i].alive) {
                return &combat.monsters[0];
            }
        }
        return nullptr;
    }
    if (waiting.empty()) {
        return nullptr;
    }
    return &waiting[0];
}

// R98 lead is Asmodeus. UNKNOWN: identification key; knownAs text is used.
bool isAsmodeusLead(const Creature* lead) {
    if (lead == nullptr) {
        return false;
    }
    return lead->knownAs == Str("Asmodeus");
}

// R98 picture layout: NoStretch centres 100x100; otherwise stretch to the view.
PictureLayout asmodeusPictureLayout(bool noStretch, i32 viewW, i32 viewH) {
    PictureLayout p;
    if (noStretch) {
        p.w = 100;
        p.h = 100;
        p.x = static_cast<i32>(vbInt(static_cast<double>(viewW - 100) / 2.0));
        p.y = static_cast<i32>(vbInt(static_cast<double>(viewH - 100) / 2.0));
    } else {
        p.x = 0;
        p.y = 0;
        p.w = viewW;
        p.h = viewH;
    }
    return p;
}

// ---------------------------------------------------------------- R99-R102 roster and look

// R99 everywhere, or the leader, or same floor and area.
// UNKNOWN: Character::level is read as location.dungeonLevel (same convention as teleporterLoadedLevel).
bool includeInRoster(const Party& party, const Character& r, bool everywhere) {
    if (everywhere) {
        return true;
    }
    const Character& leader = party.members[0];
    if (r.name == leader.name) {
        return true;  // leader always included
    }
    return r.location.dungeonLevel == leader.location.dungeonLevel && r.location.area == leader.location.area;
}

// R100 dead counted apart from living; a dead leader counts as living.
RosterCounts countRoster(const Party& party, const Vec<Character>& roster, bool everywhere) {
    RosterCounts c;
    for (u32 i = 0; i < static_cast<u32>(roster.size()); ++i) {
        const Character& r = roster[i];
        if (!includeInRoster(party, r, everywhere)) {
            continue;
        }
        const bool isLeader = r.name == party.members[0].name;
        if (r.isDead() && !isLeader) {
            c.dead += 1;
        } else {
            c.living += 1;
        }
    }
    return c;
}

// R99, R101 readout: [pos] + facing + " [ living ]" or " [ living+dead ]".
// UNKNOWN: readout position table not in SRD; lost position shown as "?" (CONFLICT R101).
// UNKNOWN: listedDead (MorgueList copy) is combined with the roster dead count by maximum.
// R-readout: location, facing and counts. Expected strings from reference tests.
// UNKNOWN: coordinate order and field names (x, y, dungeonLevel) assumed; confirm against Party.
// FIXED: Party has no member `level`; the dungeon level is read from `dungeonLevel` (same field used elsewhere in this file).
Str roomReadout(const Party& party, const Vec<Character>& roster, bool everywhere, i32 listedDead) {
    const RosterCounts c = countRoster(party, roster, everywhere);
    const i32 dead = c.dead > listedDead ? c.dead : listedDead;
    Str out;
    if (party.lostPosition) {
        // setReadout order implies (lostPosition, lostLevel, lostFacing); lost level hides the level number.
        if (party.lostLevel) {
            out += Str("(Lost)");
        } else {
            out += Str("(Lost on ") + intStr(party.dungeonLevel) + Str(")");
        }
    } else {
        out += intStr(party.x) + Str(",") + intStr(party.y) + Str(",") + intStr(party.dungeonLevel);
    }
    out += Str(" ");
    out += party.lostFacing ? Str("?") : Str(facingText(party.facing));
    if (dead > 0) {
        out += Str(" [ ") + intStr(c.living) + Str("+") + intStr(dead) + Str(" ]");
    } else {
        out += Str(" [ ") + intStr(c.living) + Str(" ]");
    }
    return out;
}

// R102 look messages: bodies (leader excluded, carried excluded) on this floor and area, and living strangers.
void reportLookAround(Ctx& ctx, const Party& party, const FloorState& floor) {
    const i32 area = currentArea(party);
    if (area < 0 || area >= kAreaCount) {
        return;
    }
    i32 bodies = 0;
    for (i32 i = 1; i < party.size; ++i) {  // leader's own body does not count
        const Character& m = party.members[static_cast<u32>(i)];
        if (!m.isDead()) {
            continue;
        }
        if (m.location.dungeonLevel != floor.dungeonLevel || m.location.area != area) {
            continue;
        }
        if (isCarriedBody(party, i)) {
            continue;
        }
        bodies += 1;
    }
    if (bodies == 1) {
        ctx.say("dungeon.lookBody", "There is a dead body here!", {}, {});
    } else if (bodies >= 2) {
        ctx.say("dungeon.lookBodies", "There are dead bodies here!", {}, {intStr(bodies)});
    }
    const Vec<Character>& occupants = floor.areas[static_cast<u32>(area)].occupants;
    bool stranger = false;
    for (u32 k = 0; k < static_cast<u32>(occupants.size()); ++k) {
        if (!occupants[k].isDead()) {
            stranger = true;  // occupants are non-party characters
            break;
        }
    }
    if (stranger) {
        ctx.say("dungeon.lookStranger", "Somebody else is in the room!", {}, {});
    }
}

// ---------------------------------------------------------------- R103-R110 bodies and carrying

// R104 candidates: dead, not self, not carried, same floor and area as picker; hidden excluded.
// UNKNOWN: hidden test uses Character::visible; data unused.
Vec<i32> pickUpCandidates(const Party& party, const FloorState& floor, const GameData& data, i32 pickerSlot) {
    (void)data;
    Vec<i32> out;
    if (pickerSlot < 0 || pickerSlot >= party.size) {
        return out;
    }
    const Character& picker = party.members[static_cast<u32>(pickerSlot)];
    for (i32 i = 0; i < party.size; ++i) {
        if (i == pickerSlot) {
            continue;
        }
        const Character& m = party.members[static_cast<u32>(i)];
        if (!m.isDead()) {
            continue;
        }
        if (isCarriedBody(party, i)) {
            continue;
        }
        if (m.location.dungeonLevel != floor.dungeonLevel || m.location.area != picker.location.area) {
            continue;
        }
        if (!m.visible) {
            continue;  // hidden cannot be picked up
        }
        out.push_back(i);
    }
    return out;
}

// R106 a put-down companion is released from its slot; a put-down body is not left in its slot.
void putDownWhatIsCarried(Ctx& ctx, Party& party, FloorState& floor, const GameData& data,
                          Vec<Companion>& companions) {
    (void)floor;
    (void)data;
    if (party.carryingSlot == CarryingSlot::Companion) {
        const i32 ci = party.carryingCompanion;
        if (ci >= 0 && ci < static_cast<i32>(companions.size())) {
            Companion& c = companions[static_cast<u32>(ci)];
            c.released = true;   // R106 released from slot
            c.occupied = false;  // UNKNOWN: occupied flag cleared on release
            c.carry = kCarryNone;
        }
        party.carryingCompanion = -1;
        party.carryingSlot = CarryingSlot::None;
        ctx.say("dungeon.putDownCompanion", "You put down your companion.", {}, {});
        return;
    }
    putDownBodyOnly(ctx, party);  // R106 body not left in its slot
}

// R107 carried bodies are delivered on walking out.
// UNKNOWN: delivery target chosen as the morgue list; the party member record is kept.
void deliverCarried(Ctx& ctx, Party& party) {
    if (party.carryingSlot != CarryingSlot::DeadCharacter) {
        return;
    }
    const i32 c = carrierIndex(party);
    if (c < 0) {
        party.carryingSlot = CarryingSlot::None;
        return;
    }
    Character& carrier = party.members[static_cast<u32>(c)];
    const Str bodyName = carrier.carryingCharacterName;
    for (i32 j = 0; j < party.size; ++j) {
        const Character& m = party.members[static_cast<u32>(j)];
        if (m.name == bodyName) {
            party.morgueList.push_back(m);
            break;
        }
    }
    carrier.carryingCharacterName = Str();
    party.carryingSlot = CarryingSlot::None;
    ctx.say("dungeon.deliver", "Your companion is delivered.", {}, {bodyName});
}

// R108 SyncAreas: area -1 at level 0. A body in rock becomes nought and is dropped from the party straight after.
// UNKNOWN: "nought" written as a blank Character record.
void syncAreas(Ctx& ctx, Party& party, Vec<FloorState>& floors, const GameData& data) {
    (void)floors;
    if (party.dungeonLevel == 0) {
        for (i32 i = 0; i < party.size; ++i) {
            party.members[static_cast<u32>(i)].location.area = -1;
        }
    }
    for (i32 i = party.size - 1; i >= 0; --i) {
        Character& m = party.members[static_cast<u32>(i)];
        if (!m.isDead()) {
            continue;
        }
        if (m.location.dungeonLevel > 0 &&
            isRockSquare(data, m.location.dungeonLevel, m.location.x, m.location.y)) {
            m = Character{};  // written as nought
            removeMember(party, i);
            ctx.say("dungeon.noughtBody", "A body is lost in the rock.", {}, {});
        }
    }
}

// R109 rescue arrival: age rises by days waited; level 0; area -1; X and Y unchanged.
void applyRescueArrival(Character& body, i32 daysWaited) {
    body.ageDays += daysWaited;
    body.location.dungeonLevel = 0;
    body.location.area = -1;
}

// R110 a saved body on rock loads on that rock.
void loadSavedBodyOnRock(Ctx& ctx, FloorState& floor, const GameData& data, Character& body) {
    const i32 idx = squareIndexOf(data, floor.dungeonLevel, body.location.x, body.location.y);
    if (idx < 0 || !isRockSquare(data, floor.dungeonLevel, body.location.x, body.location.y)) {
        return;
    }
    floor.squares[static_cast<u32>(idx)].bodies.push_back(body);
    ctx.say("dungeon.bodyOnRock", "A body lies on the rock.", {}, {body.name});
}

// ---------------------------------------------------------------- R111-R115 rescue

// R112 town shown when DungeonLevel = 0 and leader not dead.
bool townScreenWanted(const Party& party) {
    return party.dungeonLevel == 0 && !party.members[0].isDead();
}

// R112 dead screen owed only when the character is dead and alone (UBound(Chars) = 0).
bool deadScreenOwed(const Party& party) {
    return party.members[0].isDead() && party.size == 1;
}

// R113 cmdWait acts only on a dead character.
bool waitActsOn(const Character& who) {
    return who.isDead();
}

// R111 rescue arrival brings the dead body up; the town shows the dead screen.
// CONFLICT: level 0 is set by applyRescueArrival (R109).
void showRescueArrival(Ctx& ctx, Party& party, Character& body, i32 daysWaited) {
    (void)party;
    applyRescueArrival(body, daysWaited);  // R109
    ctx.say("dungeon.rescueArrival", "The body is brought up from the dungeon.", {}, {body.name});
}

// R114 threshold = ln(L)^2 * 100 + 21. UNKNOWN: ln natural; level below 1 clamped to 1 (ln 0 undefined).
double rescueThresholdDays(i32 level) {
    const double l = level < 1 ? 1.0 : static_cast<double>(level);
    const double ln = std::log(l);
    return ln * ln * 100.0 + 21.0;
}

// R114 ceiling = (days < threshold) ? 10000 : 1000; level 1 with area 18 or lower gives 100.
double rescueCeiling(i32 level, i32 area, i32 daysWaited) {
    if (level == 1 && area <= 18) {
        return 100.0;
    }
    return static_cast<double>(daysWaited) < rescueThresholdDays(level) ? 10000.0 : 1000.0;
}


// R114 found when rng.NextDouble() * ceiling <= chance.
bool rollBodyFoundToday(Ctx& ctx, i32 level, i32 area, i32 daysWaited, bool easyToFind) {
    const double ceiling = rescueCeiling(level, area, daysWaited);
    const double chance = rescueChance(level, easyToFind);
    return ctx.rng.nextDouble() * ceiling <= chance;
}

// R114 days waited, capped at the loop cap (2,000,000 days).
i32 waitForRescueDays(Ctx& ctx, i32 level, i32 area, bool easyToFind) {
    for (i32 day = 1; day <= kRescueDayCap; ++day) {
        if (rollBodyFoundToday(ctx, level, area, day, easyToFind)) {
            return day;
        }
    }
    return kRescueDayCap;
}

// ---------------------------------------------------------------- R116-R122 chests, traps, joining

// R116 one countdown step before the opener acts.
void stepChestCountdown(Ctx& ctx, Character& opener) {
    if (opener.chestCountdown > 0) {
        opener.chestCountdown -= 1;
        if (opener.chestCountdown == 0) {
            ctx.say("dungeon.chestCountdownEnd", "The guards are alerted!", {}, {opener.name});
        }
    }
}

// R116 valid opener opens an unlocked chest. With guards alive the attack order is cleared and a countdown starts if disarm > 10.
// UNKNOWN: the chest opening result (ChestOpenStart fields) is not in the API INDEX; default value returned.
ChestOpenStart beginChestAfterCombat(Ctx& ctx, Party& party, FloorState& floor,
                                     const GameData& data, i32 openerSlot) {
    if (openerSlot < 0 || openerSlot >= party.size) {
        return ChestOpenStart{};
    }
    Character& opener = party.members[static_cast<u32>(openerSlot)];
    if (opener.isDead()) {
        return ChestOpenStart{};  // UNKNOWN: valid opener taken as alive
    }
    const i32 idx = squareIndexOf(data, party.dungeonLevel, party.x, party.y);
    if (idx < 0) {
        return ChestOpenStart{};
    }
    SquareState& sq = floor.squares[static_cast<u32>(idx)];
    if (!sq.hasChest || sq.chestLock != ChestLock::UNLOCKED) {
        return ChestOpenStart{};
    }
    stepChestCountdown(ctx, opener);  // R116 countdown step first
    bool guardsAlive = false;
    for (u32 g = 0; g < 4; ++g) {
        const FoeGroup& fg = sq.foes[g];
        for (u32 m = 0; m < static_cast<u32>(fg.members.size()); ++m) {
            if (fg.members[m].alive) {
                guardsAlive = true;
            }
        }
    }
    if (guardsAlive) {
        for (u32 k = 0; k < kPartyMaxSize; ++k) {
            party.combatOrders[k].kind = CombatOrderKind::None;  // R116 attack order cleared
        }
        if (sq.chestDisarm > 10) {
            opener.chestCountdown = chestCountdownRounds(sq.chestDisarm);  // R116, R119
        }
    }
    return ChestOpenStart{};
}

// R117 a 1..100 draw at or below the chance disarms. Failure springs the trap before rewards.
// UNKNOWN: XP award and DisarmResult fields not in API INDEX; default value returned.
DisarmResult disarmChestTrap(Ctx& ctx, Party& party, Character& opener, SquareState& chest,
                             i32 bestThiefChance) {
    (void)party;
    const i32 roll = static_cast<i32>(ctx.rng.below(100)) + 1;  // R117 1..100 draw
    if (roll <= bestThiefChance) {
        chest.chestTrap = ChestTrap::NotTrapped;  // R117 disarmed
        ctx.say("dungeon.disarmed", "You disarm the trap.", {}, {opener.name});
    } else {
        // UNKNOWN: the trap effect is applied by the caller (chest.chestTrap still set).
        ctx.say("dungeon.disarmFailed", "The trap goes off!", {}, {opener.name});
    }
    return DisarmResult{};
}

// R118 chest teleporter trap, opener only.
// Unmapped: all three lost flags set. Party size > 1: opener removed, no teleport.
// Otherwise (alone): SetPosition then SettleArea with the forced flag (R89).
// UNKNOWN: invalid opener slot is a no-op returning Unmapped; choice of outcome when unmapped and alone is Unmapped.
// CONFLICT: c30 flag sources; only the chest teleporter source is set here.
ChestTeleportOutcome chestTeleporterTrap(Ctx& ctx, Party& party, Vec<FloorState>& floors,
                                         const GameData& data, i32 build,
                                         i32& flags, i32 openerSlot,
                                         i32 level, i32 x, i32 y) {
    if (openerSlot < 0 || openerSlot >= party.size) {
        return ChestTeleportOutcome::Unmapped;
    }
    const bool unmapped = isUnmappedSquare(data, build, level, x, y);  // R118 UNKNOWN test
    if (unmapped) {
        party.lostLevel = true;     // R118 all three lost flags
        party.lostPosition = true;
        party.lostFacing = true;
        party.lost = true;
    }
    if (party.size > 1) {
        removeMember(party, openerSlot);  // R118 opener removed from party
        // UNKNOWN: PruneTheDead() not called (signature not in API INDEX). No gold, no treasure.
        return ChestTeleportOutcome::OpenerRemoved;
    }
    setPartyPosition(party, level, x, y);  // R118 SetPosition(x, y)
    markForcedSettle(flags, kForceChestTeleporter);  // R89 lone opener sets the forced flag
    settleArea(ctx, party, floors, data, flags);     // R118 SettleArea
    return unmapped ? ChestTeleportOutcome::Unmapped : ChestTeleportOutcome::Teleported;
}

// R119 countdownRounds = max(2, B(10 - 2 ln(disarm - 10))) when disarm > 10.
// UNKNOWN: B(...) undefined in the dossier; VB Int (floor) used. ln natural.
i32 chestCountdownRounds(i32 disarm) {
    if (disarm <= 10) {
        return 0;
    }
    const double v = 10.0 - 2.0 * std::log(static_cast<double>(disarm - 10));
    const i32 r = static_cast<i32>(vbInt(v));
    return r < 2 ? 2 : r;
}

// R119 disarm = WIS/4 + DEX/2 + (4.5 + exp(ln(L+1)/ln 15)) * 1.4t - 9.375 * ln(max(2,M)) * ln(d+1), clamp 5..98.
// CONFLICT: integer versus real division of WIS/4 and DEX/2 is not stated; real division with VB Int.
i32 chestDisarmChance(i32 wis, i32 dex, i32 joinedGuildLevel, double thiefValue,
                      i32 guardGuildLevel, i32 depth) {
    const double l = static_cast<double>(joinedGuildLevel);
    const double m = guardGuildLevel < 2 ? 2.0 : static_cast<double>(guardGuildLevel);
    const double d = static_cast<double>(depth);
    const double v = wis / 4.0 + dex / 2.0
                   + (4.5 + std::exp(std::log(l + 1.0) / std::log(15.0))) * 1.4 * thiefValue
                   - 9.375 * std::log(m) * std::log(d + 1.0);
    i64 r = vbInt(v);
    if (r < 5) {
        r = 5;
    }
    if (r > 98) {
        r = 98;
    }
    return static_cast<i32>(r);
}

// R119 disarm XP base = t * (M/2) / (1 + active/333). UNKNOWN: party-size adjustment not implemented; VB Int rounding.
i64 disarmXpBase(double thiefValue, i32 guardGuildLevel, i32 leaderActiveLevel) {
    const double v = thiefValue * (static_cast<double>(guardGuildLevel) / 2.0)
                   / (1.0 + static_cast<double>(leaderActiveLevel) / 333.0);
    return vbInt(v);
}

// R121 base = floor(WIS/4) - floor(ln(mLvl) * ln(mDex) * 2.5)
// %chance = floor(floor(base + disarm/1.2) + ln(gLvl)^2), clamp 2..98.
// UNKNOWN: mLvl and mDex for traps; logs of values below 1 clamped to ln(1) = 0.
i32 trapIdentifyChance(i32 wis, i32 mLvl, i32 mDex, i32 gLvl, i32 disarm) {
    const double lm = std::log(mLvl >= 1 ? static_cast<double>(mLvl) : 1.0);
    const double ld = std::log(mDex >= 1 ? static_cast<double>(mDex) : 1.0);
    const double lg = std::log(gLvl >= 1 ? static_cast<double>(gLvl) : 1.0);
    const double base = static_cast<double>(vbInt(wis / 4.0)) - static_cast<double>(vbInt(lm * ld * 2.5));
    const double inner = static_cast<double>(vbInt(base + static_cast<double>(disarm) / 1.2));
    i64 r = vbInt(inner + lg * lg);
    if (r < 2) {
        r = 2;
    }
    if (r > 98) {
        r = 98;
    }
    return static_cast<i32>(r);
}

// R121 chest trap identification roll. A failed roll shows a differing guess.
bool rollTrapIdentify(Ctx& ctx, i32 wis, i32 mLvl, i32 mDex, i32 gLvl, i32 disarm) {
    const i32 chance = trapIdentifyChance(wis, mLvl, mDex, gLvl, disarm);
    const i32 roll = static_cast<i32>(ctx.rng.below(100)) + 1;
    const bool ok = roll <= chance;
    if (!ok) {
        ctx.say("dungeon.trapGuess", "You are not sure what kind of trap this is.", {}, {});
    }
    return ok;
}

// R122 join only if party index 0 is alive.
bool leaderAliveForJoin(const Party& party) {
    return party.size > 0 && !party.members[0].isDead();
}

}  // namespace mordor
