// game_setup.cpp: session start, character creation, and screen selection.
// Glue code only. Rules come from attributes.h, races.h, guilds1.h, clock.h.
//
// ASSUMPTIONS (names not shown in the API index or state structs; verify before building):
//  - loadGameDataFromJson(const Str&, GameData&, Str&) -> bool   (game_data.h)
//  - GameData::floors is a Vec-like container of FloorDef
//  - Str behaves like std::string (empty(), size(), operator[], iteration)
//  - Vec<T> has push_back()
//  - Rng is constructible from u64 and copy-assignable
//  - Ctx and Session are copyable where returned by value

#include "mordor/game.h"
#include "mordor/city1.h"
#include "mordor/attributes.h"
#include "mordor/races.h"
#include "mordor/guilds1.h"
#include "mordor/clock.h"
#include "mordor/casting1.h"

namespace mordor {
namespace {

// The rules read-only helpers take Ctx&/const Ctx&. Session::ctx() is non-const,
// so const entry points borrow a Ctx through const_cast. The rules called here
// (createBase, creationPool, statCeiling) do not mutate state.
// UNKNOWN: if statCeiling/createBase are really pure, a const Ctx constructor would be cleaner.
Ctx ctxView(const Session& s) {
    Session& m = const_cast<Session&>(s);
    return m.ctx();
}

// Name rules: non-empty and no '|' (the save format separator).
bool nameIsValid(const Str& name) {
    if (name.empty()) {
        return false;
    }
    for (char ch : name) {
        if (ch == '|') {
            return false;
        }
    }
    return true;
}

}  // namespace

bool sessionInit(Session& s, const Str& dataJson, u64 seed, Str& err) {
    // Parse the static tables first. On failure the session is left unchanged.
    if (!loadGameDataFromJson(dataJson, s.data, err)) {
        s.lastError = err;
        return false;
    }

    // Reseed the only randomness source and reset all mutable state.
    s.rng = Rng(seed);
    s.game = GameState();
    s.town = TownStock();
    { Ctx ctx = s.ctx(); seedStartingPens(ctx, s.town.pens); }   // R27: a new game starts with ten creatures in the pens

    // One FloorState per GameData floor.
    const i32 floorCount = static_cast<i32>(s.data.floors.size());
    for (i32 f = 0; f < floorCount; ++f) {
        FloorState fs;
        const FloorDef& fd = s.data.floors[static_cast<size_t>(f)];
        fs.dungeonLevel = fd.levelNum;
        fs.squares.assign(static_cast<size_t>(fd.width) * static_cast<size_t>(fd.height), SquareState());
        for (size_t q = 0; q < fs.squares.size() && q < fd.fields.size(); ++q) { fs.squares[q].areaId = fd.fields[q].areaId; fs.squares[q].bits = static_cast<u32>(fd.fields[q].flags); }
        for (size_t a = 0; a < fd.areas.size() && a < fs.areas.size(); ++a) {
            fs.areas[a].lairId = fd.areas[a].lairId;
            fs.areas[a].spawnMask = fd.areas[a].spawnMask;
        }
        fs.roomTotal = fd.headerAreaCount;
        s.game.floors.push_back(fs);
    }
    s.game.levelCount = floorCount;

    // Initialise a new game and the clocks, in that order.
    Ctx ctx = s.ctx();
    newGame(ctx, s.game);
    clockSetup(ctx, s.game);

    // R7: build the General Store shelf once the game state is reset.
    createInitialStore(ctx, s.town.store);

    s.lastError = Str();
    return true;
}

Arr<i32, 6> defaultStats(const Session& s, Race race) {
    // R2: base[i] = min(race.Max[i], race.Min[i] + 5), via the rules' createBase.
    Ctx ctx = ctxView(s);
    Arr<i32, 6> out{};
    for (i32 i = 0; i < kStatCount; ++i) {
        out[static_cast<u32>(i)] = createBase(ctx, race, static_cast<StatIndex>(i));
    }
    return out;
}

i32 creationPointsLeft(const Session& s, Race race, const Arr<i32, 6>& stats) {
    // R3: pool = 30 + race.BonusPoints - sum(stat - race.Min). Computed by the rules module.
    Ctx ctx = ctxView(s);
    return creationPool(ctx, race, stats);
}

// STARTER KIT helpers (file-local, no state).

// R3: first free pack slot scanning from internal index 1 (slot 0 is Hands).
// Empty means ItemId == 0 (SRD Pack formula: SlotEmpty).
static i32 starterFreeSlot(const Inventory& inv) {
    for (i32 i = 1; i < kPackSlots; ++i) {
        if (inv.pack[static_cast<u32>(i)].id == 0) {
            return i;
        }
    }
    return -1;
}

// Stat requirement check. statsRequired has 7 entries; index 6 is ignored,
// so only the six character stats (0..5) are compared.
static bool starterMeetsStats(const ItemDef& def, const Character& ch) {
    for (i32 i = 0; i < kStatCount; ++i) {
        const u32 idx = static_cast<u32>(i);
        if (ch.abilities.stats[idx] < def.statsRequired[idx]) {
            return false;
        }
    }
    return true;
}

// Eligibility per the starter-kit rules:
//   weapon: att > 0
//   armour: def > 0 and att == 0
//   guild bit set in item.guilds, stats met, cursed == 0.
// UNKNOWN: alignment fit is not checked (not in the starter-kit spec).
static bool starterEligible(const ItemDef& def, const Character& ch, Guild guild, bool weapon) {
    // weapons have swings and need hands; guild crests (type 34), caps etc. do not count. Armor: defence only, priced, not a crest.
    const bool kindOk = weapon ? (def.att > 0 && def.swings > 0 && def.nHands > 0 && def.price > 0) : (def.def > 0 && def.att == 0 && def.price > 0 && def.type != 34);
    if (!kindOk) {
        return false;
    }
    if (((def.guilds >> static_cast<i32>(guild)) & 1) == 0) {
        return false;
    }
    if (!starterMeetsStats(def, ch)) {
        return false;
    }
    if (def.cursed != 0) {
        return false;
    }
    return true;
}

// Index into ctx.data.items of the cheapest eligible item, or -1.
// When floorOnly is set, only items with floor <= 1 are considered.
static i32 cheapestStarter(const Ctx& ctx, const Character& ch, Guild guild, bool weapon, bool floorOnly) {
    i32 best = -1;
    for (size_t i = 0; i < ctx.data.items.size(); ++i) {
        const ItemDef& def = ctx.data.items[i];
        if (floorOnly && def.floor > 1) {
            continue;
        }
        if (!starterEligible(def, ch, guild, weapon)) {
            continue;
        }
        if (best < 0 || def.price < ctx.data.items[static_cast<size_t>(best)].price) {
            best = static_cast<i32>(i);
        }
    }
    return best;
}

// Grants one starter item (weapon or armour) into the character's pack and marks it worn.
// Preference: floor <= 1 items if any exist, otherwise any floor.
// Gold is deducted only when the item is actually granted.
static void grantStarterItem(const Ctx& ctx, Character& ch, Inventory& inv, Guild guild, bool weapon) {
    i32 pick = cheapestStarter(ctx, ch, guild, weapon, true);
    if (pick < 0) {
        pick = cheapestStarter(ctx, ch, guild, weapon, false);
    }
    if (pick < 0) {
        // UNKNOWN: no eligible starter item exists; the kit is left incomplete.
        return;
    }
    const i32 slot = starterFreeSlot(inv);
    if (slot < 0) {
        return;
    }
    const ItemDef& def = ctx.data.items[static_cast<size_t>(pick)];

    Item it;
    // UNKNOWN: Item.id is assumed to hold the table index (Hands is id 0 at table index 0).
    it.id = def.tableIndex;
    // Starter items are sold fully identified.
    it.idLevel = IdLevel::All;
    it.cursed = CurseNone;
    it.charges = def.charges;
    it.maxCharges = def.charges;
    // Worn mark: the equipped flag on the pack entry.
    // UNKNOWN: equippedWeaponSlots/wornSum source is not shown; this assumes
    // "equipped" is read from Item.equipped and inventory.worn is kept in step.
    it.equipped = true;

    inv.pack[static_cast<u32>(slot)] = it;
    // UNKNOWN: inv.worn is assumed to be the worn list used by wornSum; push_back is assumed available on Vec.
    inv.worn.push_back(it);

    // Currency is i64 in 1/10000 gold. ItemDef.price is assumed to be in gold.
    ch.wallet.goldOnHand -= static_cast<Currency>(def.price) * 10000LL;
    // UNKNOWN: inv.freeHands is not adjusted for the item's hand width (nHands); no rule given here.
}

bool createCharacter(Session& s, const NewCharacter& nc, Str& err) {
    Ctx ctx = s.ctx();

    // Validate the name.
    if (!nameIsValid(nc.name)) {
        err = "Name must be non-empty and must not contain '|'.";
        return false;
    }

    // Validate race, guild and alignment against the rules tables.
    if (!guildAllowed(ctx, nc.race, nc.guild)) {
        err = "That guild is not open to this race.";
        return false;
    }
    if (!alignmentAllowed(ctx, nc.race, nc.alignment)) {
        err = "That alignment is not open to this race.";
        return false;
    }

    // All-zero stats mean "use the default base for the race".
    Arr<i32, 6> stats = nc.stats;
    bool allZero = true;
    for (i32 i = 0; i < kStatCount; ++i) {
        if (stats[static_cast<u32>(i)] != 0) {
            allZero = false;
        }
    }
    if (allZero) {
        stats = defaultStats(s, nc.race);
    }

    // Stats must lie within race min..max.
    // SIMPLIFIED: the race minimum is not exposed by a shown function, so the lower
    // bound is checked against 1. Creation-pool arithmetic still enforces the real minimum
    // through the spinner/pool rules.
    for (i32 i = 0; i < kStatCount; ++i) {
        const i32 v = stats[static_cast<u32>(i)];
        const i32 ceiling = statCeiling(ctx, nc.race, static_cast<StatIndex>(i));
        if (v < 1 || v > ceiling) {
            err = "A stat is outside the racial range.";
            return false;
        }
    }

    // Creation points must not be overspent.
    if (creationPointsLeft(s, nc.race, stats) < 0) {
        err = "Not enough creation points.";
        return false;
    }

    // Party must have room.
    Party& party = s.game.party;
    if (party.size >= kPartyMaxSize) {
        err = "The party is full.";
        return false;
    }

    // Build the character per R5.
    Character ch;
    ch.name = nc.name;
    ch.race = nc.race;
    ch.alignment = nc.alignment;
    ch.activeGuild = nc.guild;

    for (i32 i = 0; i < kStatCount; ++i) {
        const u32 idx = static_cast<u32>(i);
        ch.abilities.stats[idx] = stats[idx];
        // R5: StatMax copied from the race maximum.
        ch.abilities.statMax[idx] = statCeiling(ctx, nc.race, static_cast<StatIndex>(i));
    }

    // R5: MaxHits = CurHits = 15.
    ch.vitals.maxHits = 15;
    ch.vitals.curHits = 15;

    // R5: 1500 gold on hand, in 1/10000 gold.
    ch.wallet.goldOnHand = 1500LL * 10000LL;

    // R5: age 16 years, expressed as kAgeStartDays.
    ch.ageDays = kAgeStartDays;

    // R5: area -1, location dungeonLevel 0.
    ch.location.area = -1;
    ch.location.dungeonLevel = 0;

    // R5: CombatSetting = 1.
    // SIMPLIFIED: no combat-setting field is shown on Character, so it is not stored.

    // One guild membership at level 1, experience 0. Apply through joinGuild, which has
    // everything it needs: game state and the inventory slot the new member will occupy.
    // UNKNOWN: joinGuild side effects on the inventory are not documented here.
    if (!joinGuild(ctx, s.game, ch, party.inventories[static_cast<u32>(party.size)], nc.guild)) {
        err = "The character could not join that guild.";
        const GuildDef& gd = ctx.data.guilds[static_cast<size_t>(nc.guild)];
        for (const Str& why : unmet(gd, ch)) {
            err += " [" + why + "]";
        }
        return false;
    }
    GuildMembership& membership = ch.guilds[static_cast<u32>(nc.guild)];
    membership.joined = true;
    membership.level = 1;
    membership.experience = 0;

    // STARTER KIT: cheapest usable weapon, then cheapest armour, placed in the new
    // member's inventory (party.inventories[party.size]) and marked worn.
    // Done after the guild join so the guild bit uses the joined guild, and before the
    // character is appended to the party so the gold deduction lands on ch.
    Inventory& kitInv = party.inventories[static_cast<u32>(party.size)];
    grantStarterItem(ctx, ch, kitInv, nc.guild, true);
    grantStarterItem(ctx, ch, kitInv, nc.guild, false);

    // R5: recalc spell points with refill, then the XP threshold.
    recalcSpellPoints(ch, true);
    ch.xpRequired = xpRequired(ctx, ch);

    // Append to the party.
    party.members[static_cast<u32>(party.size)] = ch;
    party.size++;

    s.lastError = Str();
    return true;
}

Screen currentScreen(const Session& s) {
    const Party& party = s.game.party;

    // An empty party is always the new-game screen.
    if (party.size == 0) {
        return Screen::NewGame;
    }

    // Every member dead: game over.
    bool anyAlive = false;
    for (i32 i = 0; i < party.size; ++i) {
        if (!party.members[static_cast<u32>(i)].isDead()) {
            anyAlive = true;
        }
    }
    if (!anyAlive) {
        return Screen::GameOver;
    }

    if (s.game.inCombat) {
        return Screen::Combat;
    }

    if (party.dungeonLevel > 0) {
        return Screen::Dungeon;
    }

    if (s.game.inCity) {
        return Screen::Town;
    }

    // Fallback: a living party outside the dungeon and not flagged in town is in town.
    // UNKNOWN: the SPEC does not define this case.
    return Screen::Town;
}

}  // namespace mordor
