// game_town.cpp: the town menu, townAction in game.h.
//
// Member numbers typed by the player are 1-based (1..party size).
// Pack slots are 1-based (1..39), matching the VB rule R5 that never uses slot 0.
// Gold amounts typed by the player are whole gold; Currency is 1/10000 gold.
//
// UNKNOWN: the API index gives no field name for the GameData item price, no
// equip/unequip function, no raise function and no identify function. Those
// features are listed or refused and marked SIMPLIFIED below.
#include "mordor/game.h"
#include "mordor/features.h"
#include "mordor/city1.h"
#include "mordor/casting1.h"  // FIXED: spellUse, unfitGateCheck, castCostSp, payCastCost, listReachableSpellIds, SpellUse, CastRefusal are declared here.
#include "mordor/guilds1.h"
#include "mordor/items1a.h"
#include "mordor/items1b.h"
#include "mordor/partyb.h"
#include "mordor/progression2.h"
#include "mordor/progression2b.h"
#include "mordor/store_prices.h"

namespace mordor {
namespace {

constexpr i64 kGoldToCurrency = 10000;   // Currency is 1/10000 gold
constexpr i32 kFirstPackSlot = 1;        // R5: usable pack slots are 1..39
constexpr i32 kLastPackSlot = 39;
constexpr i32 kEntryDungeonLevel = 1;

const char* const kGuildNames[kGuildCount] = {
    "nomad", "warrior", "paladin", "ninja", "villain", "seeker",
    "thief", "scavenger", "mage", "sorcerer", "wizard", "healer"};

const char* const kHelpText =
    "Town commands (members 1-based, pack slots 1-39):\n"
    "  rest | guild | levelup <member> | join <member> <guild>\n"
    "  shop | buy <row> <member> | sell <slot> <member>\n"
    "  bank deposit|withdraw <gold> (member 1's purse)\n"
    "  equip|unequip <member> <slot> | identify | morgue | raise\n"
    "  pool | descend\n";

// ---- small string helpers ----

Str toStr(i64 v) {
    if (v == 0) return Str("0");
    const bool neg = v < 0;
    u64 m = neg ? static_cast<u64>(-(v + 1)) + 1u : static_cast<u64>(v);
    Str out;
    while (m > 0) {
        out.insert(out.begin(), static_cast<char>('0' + static_cast<i32>(m % 10)));
        m /= 10;
    }
    if (neg) out.insert(out.begin(), '-');
    return out;
}

Str lowered(const Str& w) {
    Str r = w;
    for (u32 i = 0; i < r.size(); ++i) {
        if (r[i] >= 'A' && r[i] <= 'Z') r[i] = static_cast<char>(r[i] - 'A' + 'a');
    }
    return r;
}

Vec<Str> splitWords(const Str& text) {
    Vec<Str> out;
    Str cur;
    for (u32 i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == ' ' || c == '\t') {
            if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
        } else {
            cur += c;
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

bool parseNum(const Str& w, i64& out) {
    if (w.empty()) return false;
    u32 i = 0;
    bool neg = false;
    if (w[0] == '-') {
        neg = true;
        i = 1;
    }
    if (i >= w.size()) return false;
    i64 v = 0;
    for (; i < w.size(); ++i) {
        const char c = w[i];
        if (c < '0' || c > '9') return false;
        v = v * 10 + (c - '0');
        if (v > 1000000000000LL) return false;
    }
    out = neg ? -v : v;
    return true;
}

// Player member number (1-based) to 0-based index.
bool memberIndex(const Session& s, const Str& w, i32& idx) {
    i64 n = 0;
    if (!parseNum(w, n) || n < 1 || n > s.game.party.size) return false;
    idx = static_cast<i32>(n - 1);
    return true;
}

// Player pack slot (1-based, 1..39) kept as the pack index.
bool packSlot(const Str& w, i32& slot) {
    i64 n = 0;
    if (!parseNum(w, n) || n < kFirstPackSlot || n > kLastPackSlot) return false;
    slot = static_cast<i32>(n);
    return true;
}

bool guildFromWord(const Str& w, Guild& g) {
    i64 n = 0;
    if (parseNum(w, n)) {
        if (n < 0 || n >= kGuildCount) return false;
        g = static_cast<Guild>(n);
        return true;
    }
    const Str lw = lowered(w);
    for (i32 i = 0; i < kGuildCount; ++i) {
        if (lw == Str(kGuildNames[i])) {
            g = static_cast<Guild>(i);
            return true;
        }
    }
    return false;
}

Str guildName(Guild g) {
    return Str(kGuildNames[static_cast<i32>(g)]);
}

// ---- access helpers ----

Character& memberAt(Party& p, i32 i) {
    return p.members[static_cast<u32>(i)];
}

const Character& memberAt(const Party& p, i32 i) {
    return p.members[static_cast<u32>(i)];
}

Inventory& invAt(Party& p, i32 i) {
    return p.inventories[static_cast<u32>(i)];
}

// FIXED: removed local firstFreePackSlot(const Inventory&); it had the same signature as
// the declaration in items1a.h, so calls were ambiguous. The header declaration is used.

// Every line of output goes to the reply and to the event log.
void note(Ctx& ctx, Str& reply, const char* key, const Str& text) {
    reply += text;
    reply += '\n';
    ctx.say(key, text, {}, {});
}

bool usage(Ctx& ctx, Str& reply, const char* form) {
    note(ctx, reply, "town.usage", Str("Usage: ") + form);
    return true;
}

// ---- actions ----

// R3: restInCity for every member, dead included (the rule decides).
void doRest(Ctx& ctx, Session& s, Str& reply) {
    Party& party = s.game.party;
    i32 firstMemberDays = 0;
    for (i32 i = 0; i < party.size; ++i) {
        Character& ch = memberAt(party, i);
        const i32 hitsBefore = ch.vitals.curHits;
        const i32 days = restInCity(ctx, ch);   // days aged: round_half_even((100 - hits%)/3.25), 0 when unhurt
        if (i == 0) firstMemberDays = days;
        const i32 restored = ch.vitals.curHits - hitsBefore;
        note(ctx, reply, "town.rest",
             ch.name + ": aged " + toStr(days) + " day(s), hits restored " + toStr(restored) +
                 ", now " + toStr(ch.vitals.curHits) + "/" + toStr(ch.vitals.maxHits));
    }
    // Reference rest (CityWindow): only the FIRST member's rest turns the store and the pens over, and only if it aged days.
    if (firstMemberDays > 0) {
        replenishStore(ctx, s.town.store, firstMemberDays, s.town.deepestReached);
        restockPens(ctx, s.town.pens, firstMemberDays, s.town.deepestReached);
    }
}

void doGuildList(Ctx& ctx, const Party& party, Str& reply) {
    for (i32 i = 0; i < party.size; ++i) {
        const Character& ch = memberAt(party, i);
        note(ctx, reply, "town.guilds", ch.name + " (active " + guildName(ch.activeGuild) + ")");
        for (i32 g = 0; g < kGuildCount; ++g) {
            const Guild gu = static_cast<Guild>(g);
            const GuildMembership& m = ch.guilds[static_cast<u32>(g)];
            Str line = "  " + guildName(gu);
            if (m.joined) {
                // UNKNOWN: xp needed taken as Character.xpRequired minus the guild's experience.
                i64 need = ch.xpRequired - m.experience;
                if (need < 0) need = 0;
                // UNKNOWN: goldForNextLevel units taken as Currency (1/10000 gold); shown raw.
                line += ": level " + toStr(m.level) + ", xp needed " + toStr(need) +
                        ", gold for next level " + toStr(goldForNextLevel(ch, gu));
            } else {
                line += ": not joined, join cost " + toStr(guildJoinCost(gu));
            }
            note(ctx, reply, "town.guilds", line);
        }
    }
}

// R (levelup): refuse unless canLevelUp, then makeLevel for the active guild.
void doLevelUp(Ctx& ctx, Session& s, i32 idx, Str& reply) {
    Party& party = s.game.party;
    Character& ch = memberAt(party, idx);
    const Guild g = ch.activeGuild;
    const Str gname = guildName(g);
    if (ch.isDead()) {
        note(ctx, reply, "town.levelup", ch.name + " is dead and cannot level up.");
        return;
    }
    if (!ch.guilds[static_cast<u32>(g)].joined) {
        note(ctx, reply, "town.levelup", ch.name + " is not a member of " + gname + ".");
        return;
    }
    if (!canLevelUp(ch, g)) {
        note(ctx, reply, "town.levelup",
             ch.name + " cannot level " + gname +
                 ": not enough experience or gold (gold for next level " +
                 toStr(goldForNextLevel(ch, g)) + ").");
        return;
    }
    const i32 before = ch.guilds[static_cast<u32>(g)].level;
    makeLevel(ctx, s.game, ch, invAt(party, idx), g);
    const i32 after = ch.guilds[static_cast<u32>(g)].level;
    if (after > before) {
        note(ctx, reply, "town.levelup",
             ch.name + " levels " + gname + ": " + toStr(before) + " -> " + toStr(after) + ".");
    } else {
        note(ctx, reply, "town.levelup", "Level-up refused by the rules for " + ch.name + ".");
    }
}

void doJoin(Ctx& ctx, Session& s, i32 idx, Guild g, Str& reply) {
    Party& party = s.game.party;
    Character& ch = memberAt(party, idx);
    if (ch.guilds[static_cast<u32>(g)].joined) {
        note(ctx, reply, "town.join", ch.name + " is already in " + guildName(g) + ".");
        return;
    }
    if (joinGuild(ctx, s.game, ch, invAt(party, idx), g)) {
        note(ctx, reply, "town.join", ch.name + " joined " + guildName(g) + ".");
    } else {
        note(ctx, reply, "town.join",
             "Join refused for " + ch.name + " (join cost " + toStr(guildJoinCost(g)) +
                 "; not enough gold or not eligible).");
    }
}

// SIMPLIFIED: prices are not listed; the GameData item price field is not in the API shown.
// UNKNOWN: Item has no count member, so each shelf entry is listed as one row.
// UNKNOWN: Item.id is matched to ItemDef.id; the table index is not used as a fallback.
static const ItemDef* findItemDef(const GameData& data, i32 id) {
    for (size_t i = 0; i < data.items.size(); ++i) {
        if (data.items[i].id == id) {
            return &data.items[i];
        }
    }
    return nullptr;
}

// ---- src/game_town.cpp :: doShop
// Helpers for the General Store. This block supersedes the earlier doShop block and contains
// every helper the store needs (doBuy and doSell below use them). Do not keep an older copy.
// Requires <cmath>, math_util.h (roundHalfEven), items2a.h (packIndexFromPlayerSlot,
// playerSlotFromPackIndex) to be visible in game_town.cpp.

// R? General Store price cap on the shelf count (StoreStock.ShelfCap = 9).
constexpr i32 kStoreShelfCap = 9;

// Number of copies of this item id and alignment on the shelf (StoreStock.Count).
static i32 storeShelfCount(const Vec<Item>& store, const Item& it) {
    i32 n = 0;
    for (size_t i = 0; i < store.size(); ++i) {
        if (store[i].id == it.id && store[i].alignment == it.alignment) ++n;
    }
    return n;
}

// What the store charges to sell one copy to you, in whole gold (StoreStock.PriceOf).
// Floor-0 items sell at the list price. Otherwise the price depends on the shelf count.
// UNKNOWN: no store price function is visible in the evidence packet, so the reference
// formula is applied here directly. Replace with the module function when one is available.
static i64 storeBuyPriceGold(const GameData& data, const Vec<Item>& store, const Item& it) {
    return static_cast<i64>(storeBuyPrice(data, it, storeShelfCount(store, it)) / 10000);
}

// Whole gold with up to four decimals (1/10000 gold units), trailing zeros dropped.
static Str formatGold(Currency units) {
    const Currency perGold = 10000;
    const bool negative = units < 0;
    const Currency magnitude = negative ? -units : units;
    const Currency whole = magnitude / perGold;
    Currency frac = magnitude % perGold;
    Str out = negative ? Str("-") : Str();
    out += toStr(whole);
    if (frac != 0) {
        Str digits(4, '0');
        for (int d = 3; d >= 0; --d) {
            digits[static_cast<size_t>(d)] = static_cast<char>('0' + static_cast<int>(frac % 10));
            frac /= 10;
        }
        while (!digits.empty() && digits.back() == '0') digits.pop_back();
        out += "." + digits;
    }
    out += " gold";
    return out;
}

// True when store[r] is the first copy of its item id on the shelf (a shop row starts here).
static bool storeIsFirstOfItem(const Vec<Item>& store, size_t r) {
    for (size_t q = 0; q < r; ++q) {
        if (store[q].id == store[r].id) return false;
    }
    return true;
}

// Shelf index of the first copy of the given 1-based shop row, or -1 if there is no such row.
// Rows are the distinct item ids in shelf order (the same rule doShop uses to number them).
static i64 storeRowIndex(const Vec<Item>& store, i64 row) {
    if (row < 1) return -1;
    i64 n = 0;
    for (size_t r = 0; r < store.size(); ++r) {
        if (!storeIsFirstOfItem(store, r)) continue;
        ++n;
        if (n == row) return static_cast<i64>(r);
    }
    return -1;
}

// FIXED: packIndexFromPlayerSlot and playerSlotFromPackIndex are not in any header, so they are
// defined here as static, before doSell and doBuy use them. Player slot 1 is the hands (pack
// index 0), so the player slot is the pack index plus one.
static i32 packIndexFromPlayerSlot(i32 playerSlot) {
    return playerSlot - 1;
}

static i32 playerSlotFromPackIndex(i32 packIndex) {
    return packIndex + 1;
}

// ---- src/game_town.cpp :: doShop
// General store listing. Prices come from the module function storeBuyPriceGold, so no
// price helper is defined here. Do not keep an earlier static storeBuyPriceGold: it would
// clash with the module declaration.
void doShop(Ctx& ctx, Session& s, Str& reply) {
    note(ctx, reply, "town.shop", "General store stock (buy <row> <member>; rows are numbered from 1):");
    const size_t total = s.town.store.size();
    if (total == 0) {
        note(ctx, reply, "town.shop", "  (the shelves are empty)");
        return;
    }
    // Rows are the distinct item ids in shelf order, numbered 1..N with no gaps.
    // The row number is the one that buy takes. The row shows the price of its first copy.
    i64 row = 0;
    for (size_t r = 0; r < total; ++r) {
        if (!storeIsFirstOfItem(s.town.store, r)) continue;
        const Item& item = s.town.store[r];
        i64 count = 0;
        for (size_t q = 0; q < total; ++q) {
            if (s.town.store[q].id == item.id) ++count;
        }
        ++row;
        const ItemDef* def = findItemDef(s.data, item.id);
        const Str name = def != nullptr ? def->name : Str("?");
        const Str price = def != nullptr ? toStr(storeBuyPriceGold(s.data, s.town.store, item)) : Str("0");
        note(ctx, reply, "town.shop",
             "  " + toStr(row) + ": " + name + "  " + price + " gold  x" + toStr(count));
    }
}

// SIMPLIFIED: the store row -> Item mapping and the GameData price are not in the API shown,
// so nothing is charged and no item moves. Only the row check and free-slot search are done.
// ---- src/game_town.cpp :: doBuy
void doBuy(Ctx& ctx, Session& s, i64 row, i32 idx, Str& reply) {
    Party& party = s.game.party;
    if (idx < 0 || idx >= party.size || idx >= kPartyMaxSize) {
        note(ctx, reply, "town.buy", "No such party member.");
        return;
    }

    // The row is a shop row (distinct item), not a raw shelf index. Map it to the first copy.
    const i64 rowIndexSigned = storeRowIndex(s.town.store, row);
    if (rowIndexSigned < 0) {
        note(ctx, reply, "town.buy", "No store row " + toStr(row) + ".");
        return;
    }
    const size_t rowIndex = static_cast<size_t>(rowIndexSigned);
    const Item shelfItem = s.town.store[rowIndex];

    const ItemDef* def = findItemDef(s.data, shelfItem.id);
    if (def == nullptr) {
        note(ctx, reply, "town.buy", "That item is not in the item table.");
        return;
    }

    // R? General Store buy price: stock-dependent, charged in whole gold (StoreStock.PriceOf).
    // The price is taken before the copy leaves the shelf, so the shelf count includes it.
    const i64 priceGold = storeBuyPriceGold(s.data, s.town.store, shelfItem);
    // Currency is gold * 10000.
    const Currency kCurrencyPerGold = 10000;
    const Currency cost = static_cast<Currency>(priceGold) * kCurrencyPerGold;

    // Pack slots internal indices 1..39 are used; index 0 is not examined (R5).
    // UNKNOWN: firstFreePackSlot is assumed to return an index into Inventory.pack.
    Inventory& inv = party.inventories[static_cast<size_t>(idx)];
    const i32 freeSlot = firstFreePackSlot(inv);
    if (freeSlot < 1 || freeSlot >= kPackSlots) {
        note(ctx, reply, "town.buy", "No free pack slot for " + party.members[static_cast<size_t>(idx)].name + ".");
        return;
    }

    Character& who = party.members[static_cast<size_t>(idx)];
    if (who.wallet.goldOnHand < cost) {
        note(ctx, reply, "town.buy", who.name + " cannot pay " + toStr(priceGold) + " gold for " + def->name + ".");
        return;
    }

    // Pay, move a copy into the pack, then remove one copy from the shelf.
    who.wallet.goldOnHand -= cost;
    Item bought = shelfItem;
    bought.equipped = false;
    inv.pack[static_cast<size_t>(freeSlot)] = bought;

    // Rebuild the shelf without the bought copy (Vec has no known erase).
    Vec<Item> remaining;
    for (size_t i = 0; i < s.town.store.size(); ++i) {
        if (i != rowIndex) {
            remaining.push_back(s.town.store[i]);
        }
    }
    s.town.store = remaining;

    // Report the player-facing slot number (1..40), the same number doSell accepts.
    const i32 playerSlot = playerSlotFromPackIndex(freeSlot);
    note(ctx, reply, "town.buy",
         who.name + " buys " + def->name + " for " + toStr(priceGold)
         + " gold into pack slot " + toStr(static_cast<i64>(playerSlot)) + ".");
}

// ch.16 sale rule (half of price) is applied by storeSale.
// UNKNOWN: whether the gold goes to the named member or the leader; the purse gain of the
// named member decides success. The sold item is cleared only when gold was gained.
// ---- src/game_town.cpp :: doSell
// SIGNATURE CHANGED: now takes Session& so the sold item can go on s.town.store.
// The townAction call site must pass s (see NEED below). The prototype, if any, must match.
// Whole gold with up to four decimals, trailing zeros dropped (e.g. "12 gold", "0.5 gold").
// Never prints raw 1/10000 units.
static Str storeGoldText(Currency units) {
    const Currency perGold = 10000;
    const bool negative = units < 0;
    const Currency magnitude = negative ? -units : units;
    const Currency whole = magnitude / perGold;
    Currency frac = magnitude % perGold;
    Str out = negative ? Str("-") : Str();
    out += toStr(static_cast<i64>(whole));
    if (frac != 0) {
        Str digits;
        for (int d = 0; d < 4; ++d) {
            digits.push_back(static_cast<char>('0' + static_cast<int>(frac / 1000)));
            frac = (frac % 1000) * 10;
        }
        while (!digits.empty() && digits.back() == '0') digits.pop_back();
        out += "." + digits;
    }
    out += " gold";
    return out;
}

// FIXED: removed the second, identical definition of storeGoldText (it was a redefinition
// of the function above). The single definition above is used by doSell.

// Sell to the General Store. The payment is the reference SellPrice (StoreStock.cs:99),
// obtained through the module function sellQuote. storeSale is not used because its payout
// (generalStorePayout, R17) does not match the reference.
// CONFLICT: city1_a.cpp storeSale pays generalStorePayout(price); the reference pays SellPrice.
// UNKNOWN: whether an item the store does not shelve is still taken and paid for; this code
// takes it and pays (the reference SellPrice is paid whenever the store buys).
// UNKNOWN: TownIncomeBank payee redirection (townIncomePayee) is not applied here; the seller is paid.
void doSell(Ctx& ctx, Session& s, i32 slot, i32 idx, Str& reply) {
    Party& party = s.game.party;
    if (idx < 0 || idx >= party.size || idx >= kPartyMaxSize) {
        note(ctx, reply, "town.sell", "No such party member.");
        return;
    }
    // The sell slot is the player-facing pack slot (1..40), the same number doBuy reports.
    // Convert it to the internal pack index (0..39); index 0 is not examined (R5).
    const i32 index = packIndexFromPlayerSlot(slot);
    if (index < 1 || index >= kPackSlots) {
        note(ctx, reply, "town.sell", "No pack slot " + toStr(static_cast<i64>(slot)) + ".");
        return;
    }

    Item& item = invAt(party, idx).pack[static_cast<size_t>(index)];
    if (item.id == 0) {
        note(ctx, reply, "town.sell", "Pack slot " + toStr(static_cast<i64>(slot)) + " is empty.");
        return;
    }

    const ItemDef* def = findItemDef(s.data, item.id);
    const Str name = def != nullptr ? def->name : Str("item");

    // Keep a copy of the item as it was offered; the pack entry is cleared only on success.
    Item soldCopy = item;

    // R? General Store sell price: the reference SellPrice, quoted by the module.
    Currency quote = 0;
    if (!sellQuote(ctx, soldCopy, quote) || quote <= 0) {
        note(ctx, reply, "town.sell", "Sale refused: the store will not buy " + name + ".");
        return;
    }

    Character& seller = party.members[static_cast<size_t>(idx)];
    const Currency kCurrencyCeiling = 9223372036854775807LL;
    if (seller.wallet.goldOnHand > kCurrencyCeiling - quote) {
        note(ctx, reply, "town.sell", "Sale refused: " + seller.name + " cannot hold that much gold.");
        return;
    }

    // Pay the seller, then shelve the item if the store takes it (the store grows by one).
    seller.wallet.goldOnHand += quote;
    const bool shelves = storeShelvesItem(ctx, soldCopy, s.town.store);
    soldCopy.equipped = false;
    if (shelves) {
        s.town.store.push_back(soldCopy);
    }
    item = Item{};

    ctx.say("city.storeSale", "Item sold to the General Store",
            {quote, static_cast<i64>(idx)}, {});

    if (shelves) {
        note(ctx, reply, "town.sell",
             "Sold " + name + " for " + storeGoldText(quote) + " and put it on the shelf.");
    } else {
        note(ctx, reply, "town.sell",
             "Sold " + name + " for " + storeGoldText(quote) + ". The store does not shelve it.");
    }
}

// Bank works on member 1's purse. Deposit uses depositGold.
// SIMPLIFIED: withdraw moves gold locally; no withdraw-gold rule is in the API index.
void doBank(Ctx& ctx, Party& party, bool deposit, i64 gold, Str& reply) {
    if (gold <= 0) {
        note(ctx, reply, "town.bank", "Amount must be positive.");
        return;
    }
    // Overflow guard: gold * kGoldToCurrency must fit in i64 (no exceptions, no UB).
    const i64 kMaxI64 = 9223372036854775807LL;
    if (gold > kMaxI64 / kGoldToCurrency) {
        note(ctx, reply, "town.bank", "Amount too large.");
        return;
    }
    const Currency amount = gold * kGoldToCurrency;
    Purse& purse = memberAt(party, 0).wallet;
    if (deposit) {
        // The bank takes gold the character holds on hand (the bank's Party Deposit moves
        // on-hand gold, Party.cs:239). Refuse explicitly rather than let depositGold fail silently.
        if (purse.goldOnHand < amount) {
            note(ctx, reply, "town.bank",
                 "Deposit refused: you have only " +
                     toStr(purse.goldOnHand / kGoldToCurrency) + " gold on hand.");
            return;
        }
        // UNKNOWN: the SRD ch16 bank balance cap is not in the packet. It is assumed to be
        // enforced inside depositGold; its refusal is reported without guessing the reason.
        if (depositGold(ctx, purse, amount) == BankResult::Ok) {
            note(ctx, reply, "town.bank",
                 "Deposited " + toStr(gold) + " gold. Bank balance: " +
                     toStr(purse.goldInBank / kGoldToCurrency) + " gold.");
        } else {
            note(ctx, reply, "town.bank",
                 "Deposit refused: the bank did not take " + toStr(gold) + " gold.");
        }
        return;
    }
    if (purse.goldInBank < amount) {
        note(ctx, reply, "town.bank",
             "Withdraw refused: not enough gold in the bank (balance " +
                 toStr(purse.goldInBank / kGoldToCurrency) + " gold).");
        return;
    }
    // Overflow guard: goldOnHand + amount must fit in i64 before anything moves.
    if (amount > kMaxI64 - purse.goldOnHand) {
        note(ctx, reply, "town.bank", "Withdraw refused: it would overflow the gold on hand.");
        return;
    }
    purse.goldInBank -= amount;
    purse.goldOnHand += amount;
    note(ctx, reply, "town.bank",
         "Withdrew " + toStr(gold) + " gold. On hand: " +
             toStr(purse.goldOnHand / kGoldToCurrency) + " gold.");
}

// SIMPLIFIED: the items module equip/unequip functions are not in the API index; nothing changes.
// (Handled inline in townAction.)

// SIMPLIFIED: lists the fee for each unidentified pack item; no identification is done.
void doIdentify(Ctx& ctx, Party& party, Str& reply) {
    i32 listed = 0;
    for (i32 i = 0; i < party.size; ++i) {
        const Character& ch = memberAt(party, i);
        for (i32 k = kFirstPackSlot; k <= kLastPackSlot; ++k) {
            const Item& it = invAt(party, i).pack[static_cast<u32>(k)];
            if (it.id == 0) continue;
            if (static_cast<i32>(it.idLevel) >= static_cast<i32>(IdLevel::All)) continue;
            const Currency fee = identifyFee(ctx, it);
            ++listed;
            note(ctx, reply, "town.identify",
                 ch.name + " slot " + toStr(k) + ": identify fee " + toStr(fee));
        }
    }
    if (listed == 0) {
        note(ctx, reply, "town.identify", "Nothing to identify.");
    }
}

// SIMPLIFIED: lists dead members; raising needs a death-module call not in the API index.
void doMorgue(Ctx& ctx, const Party& party, Str& reply) {
    // R (ch18): the Morgue lists the dead. Raising needs a raise call that is not in the API index.
    i32 dead = 0;
    note(ctx, reply, "town.morgue", "Morgue (dead members):");
    for (i32 i = 0; i < party.size; ++i) {
        const Character& ch = memberAt(party, i);
        if (!ch.isDead()) continue;
        ++dead;
        note(ctx, reply, "town.morgue",
             "  " + toStr(i + 1) + ": " + ch.name + ", state " +
                 toStr(static_cast<i32>(ch.deathState)));
    }
    if (dead == 0) {
        note(ctx, reply, "town.morgue", "  nobody is dead.");
        return;
    }
    // UNKNOWN: raise cost, success chance and consequences are not in the packet (ch18 excerpt
    // missing). Refusing explicitly until the raise call is wired, rather than charging a guessed fee.
    note(ctx, reply, "town.morgue",
         "Raising is not available yet: the raise rules are not wired. Nobody was raised and no gold was taken.");
}

// R24: pool every other member's on-hand gold into the selected member. The amount moved is
// measured from the selected member's purse, so the reply is exact even when pooling stops.
void doPool(Ctx& ctx, Party& party, Str& reply) {
    const i32 to = party.selectedIndex;
    const Currency before = party.members[to].wallet.goldOnHand;
    const bool ok = poolGoldToSelected(ctx, party);
    const Currency moved = party.members[to].wallet.goldOnHand - before;
    const Str who = party.members[to].name;
    if (ok) {
        note(ctx, reply, "town.pool",
             "Pooled " + toStr(moved / kGoldToCurrency) + " gold on hand into " + who + ".");
    } else {
        // C#: an overflow stops pooling. Members already pooled stay pooled; the rest keep their gold.
        note(ctx, reply, "town.pool",
             "Pooling stopped at an overflow: " + who + "'s purse cannot take the next member's gold. " +
                 toStr(moved / kGoldToCurrency) + " gold was pooled before it stopped.");
    }
}

// SIMPLIFIED: the level-1 entry square is not in the API shown, so the party is placed at 0,0
// facing N. leaveArea is not called: it is the dungeon-side exit rule.
// Scans level 1 for the FLAG_STAIRS_UP square (flags bit 12, index y*width+x).
static bool findStairsUpSquare(const GameData& data, i32& outX, i32& outY) {
    constexpr u32 kFlagStairsUp = 1u << 12;  // stairs-up bit in floor field flags
    if (data.floors.size() == 0) return false;
    const i32 width = data.floors[0].width;  // assumed field name, see note
    if (width <= 0) return false;
    const i32 count = static_cast<i32>(data.floors[0].fields.size());
    for (i32 i = 0; i < count; ++i) {
        const u32 flags = static_cast<u32>(data.floors[0].fields[i].flags);  // assumed field name
        if ((flags & kFlagStairsUp) != 0u) {
            outX = i % width + 1;   // 1-based like the dungeon module
            outY = i / width + 1;
            return true;
        }
    }
    return false;
}

void doDescend(Ctx& ctx, Session& s, Str& reply) {
    s.game.underground = true;   // the combat clock runs only below ground (SRD ch.6 R1)
    Party& party = s.game.party;
    if (s.game.inDungeon || party.inDungeon) {
        note(ctx, reply, "town.descend", "The party is already in the dungeon.");
        return;
    }
    bool anyAlive = false;
    for (i32 i = 0; i < party.size; ++i) {
        if (!memberAt(party, i).isDead()) anyAlive = true;
    }
    if (party.size == 0 || !anyAlive) {
        note(ctx, reply, "town.descend", "No living party member to descend.");
        return;
    }

    // Arrival square is the level-1 stairs up, not (0,0).
    i32 arrivalX = 0;
    i32 arrivalY = 0;
    if (!findStairsUpSquare(ctx.data, arrivalX, arrivalY)) {
        note(ctx, reply, "town.descend", "No stairs up were found on dungeon level 1.");
        return;
    }

    // UNKNOWN: wall edge bits are not in the provided structs, so the facing
    // is not checked against walls. North is used.
    const Facing arrivalFacing = Facing::N;

    party.inDungeon = true;
    party.dungeonLevel = kEntryDungeonLevel;
    if (kEntryDungeonLevel > s.town.deepestReached) s.town.deepestReached = kEntryDungeonLevel;   // restock input
    party.x = arrivalX;
    party.y = arrivalY;
    party.facing = arrivalFacing;
    party.onStairs = true;
    party.lost = false;
    party.lostFacing = false;
    party.lostLevel = false;
    party.lostPosition = false;

    s.game.inCity = false;
    s.game.inDungeon = true;
    s.game.dungeonLevel = kEntryDungeonLevel;
    s.game.partyPosition.dungeonLevel = kEntryDungeonLevel;
    s.game.partyPosition.x = arrivalX;
    s.game.partyPosition.y = arrivalY;

    for (i32 i = 0; i < party.size; ++i) {
        Location& loc = memberAt(party, i).location;
        loc.dungeonLevel = kEntryDungeonLevel;
        loc.x = arrivalX;
        loc.y = arrivalY;
        loc.facing = static_cast<i32>(arrivalFacing);
        // UNKNOWN: loc.area should come from the arrival square's areaId, and
        // the square should be marked explored. The floor-state member that
        // holds SquareState is not in the provided structs, so both are not
        // set here.
    }
    note(ctx, reply, "town.descend", "The party descends to dungeon level 1.");
}

}  // namespace

// FIXED: IdentifyOutcome is not in any header. It is used only by identifyItem and this file,
// so it is defined here with the four outcomes this file handles.
enum class IdentifyOutcome : i32 {
    EmptySlot = 0,
    NotNeeded,
    CannotAfford,
    Identified
};

// FIXED: identifyItem is not in any header. Its definition is in items2b_2.cpp (per the comment
// on doIdentifyItem), so it is declared with external linkage to match that definition.
// A static declaration would not link to it.
IdentifyOutcome identifyItem(Ctx& ctx, Character& payer, Item& item);

// FIXED: forward declaration; doIdentifyItem is defined below townAction and is not in any header.
static void doIdentifyItem(Ctx& ctx, Party& party, i32 member, i32 slot, Str& reply);

// FIXED: forward declarations for the gear helpers, which are defined further down this file
// but are used by the gear verbs below. Declared static to match their definitions.
static bool gearSlotOk(i32 playerSlot, Str& reply);
static Str gearNum(i32 v);
static Str gearItemName(const Ctx& ctx, const Item& it);
static Str gearAdLine(const Ctx& ctx, const Character& c, const Inventory& inv);

// Usage line for the gear verbs. Plain text, so note() can take it directly.
static const char* gearUsageText(const Str& verb) {
    if (verb == "equip") return "Usage: equip <member> <slot>";
    if (verb == "use") return "Usage: use <member> <slot>";
    return "Usage: unequip <member> <slot>";
}

// Gear helpers (R7, R13 via items1b.cpp, items3a.cpp, attributes.cpp). No rule is re-implemented here.

// Decimal text for an i32 without a header dependency.
static Str numText(i32 v) {
    if (v == 0) return "0";
    const bool neg = v < 0;
    i64 n = neg ? -static_cast<i64>(v) : static_cast<i64>(v);
    Str out;
    while (n > 0) {
        out.insert(out.begin(), static_cast<char>('0' + n % 10));
        n /= 10;
    }
    return neg ? Str("-") + out : out;
}

// Recomputed on every gear command; nothing is cached.
static Str adSummary(const Ctx& ctx, const Character& c, const Inventory& inv) {
    const AdPair t = totalAD(ctx, c, inv);
    const i32 combat = combatAD(ctx, c, inv);
    return Str("Combat A/D ") + numText(combat) + ", total attack " + numText(t.att) + ", total defence " + numText(t.def) + ".";
}

// FIXED: itemNameOf is not in any header. The name is the ItemDef name for the item id,
// or "?" when the id is not in the item table.
static Str itemNameOf(const Ctx& ctx, const Item& it) {
    const ItemDef* def = findItemDef(ctx.data, it.id);
    return def != nullptr ? def->name : Str("?");
}

// FIXED: itemSlotCursed is not in any header and no curse field is in the API index.
// Stub: reports no curse, so the cursed refusals never fire until the rule is wired.
static bool itemSlotCursed(const Item&) {
    return false;
}

// FIXED: itemUseProblem is not in any header. Stub: reports UseCheck::Ok, so the
// out-of-charges refusal never fires until the item-use rule is wired.
static i32 itemUseProblem(const Ctx&, const Character&, const Item&) {
    return static_cast<i32>(UseCheck::Ok);
}

// FIXED: applyItemUse is not in any header. Stub: refuses the use until the item-use call is wired.
static bool applyItemUse(Ctx&, Session&, i32, i32, Str& reply) {
    reply = "Using items is not wired yet. Nothing was used.\n";
    return false;
}

// R7 wearing check and the equip path. Order (hands, too many, guild, guild level, stats)
// is enforced by equipProblem in items1b.cpp; this function only reports the result.
static void gearEquip(Ctx& ctx, Session& s, i32 idx, i32 playerSlot, Str& reply) {
    if (!gearSlotOk(playerSlot, reply)) return;
    const i32 pack = packIndexFromPlayerSlot(playerSlot);
    Character& c = s.game.party.members[static_cast<size_t>(idx)];
    // FIXED: inventoryOf is not declared anywhere; the member's inventory is party.inventories
    // at the same index, reached through the existing invAt helper.
    Inventory& inv = invAt(s.game.party, idx);
    Item& it = inv.pack[pack];
    if (it.id <= 0) {
        reply = Str("Pack slot ") + gearNum(playerSlot) + " is empty.\n";
        return;
    }
    const Str name = gearItemName(ctx, it);
    if (it.equipped) {
        reply = name + " is already worn. Use unequip to take it off.\n";
        return;
    }
    // Cast selects the const Ctx& overload in items1b (returns EquipProblem), not the
    // Ctx& overload in items1a (returns bool).
    const EquipProblem p = equipProblem(static_cast<const Ctx&>(ctx), c, inv, it);
    if (p != EquipProblem::None) {
        reply = name + " cannot be equipped: " + equipProblemText(p) + "\n";
        return;
    }
    if (!equipItem(ctx, c, inv, pack)) {
        reply = name + " could not be equipped.\n";
        return;
    }
    reply = Str("Member ") + gearNum(idx + 1) + " equipped " + name + " from pack slot " + gearNum(playerSlot) + ". ";
    // UNKNOWN: the reference gives no wording for an item that becomes cursed on wearing;
    // the curse is reported plainly.
    if (itemSlotCursed(it)) {
        reply += name + " is cursed and cannot be taken off. ";
    }
    reply += gearAdLine(ctx, c, inv) + "\n";
}

// Removal. A cursed worn item stays on (CanUnequip: !slot.SlotCursed, Character.cs).
static void gearUnequip(Ctx& ctx, Session& s, i32 idx, i32 playerSlot, Str& reply) {
    if (!gearSlotOk(playerSlot, reply)) return;
    const i32 pack = packIndexFromPlayerSlot(playerSlot);
    Character& c = s.game.party.members[static_cast<size_t>(idx)];
    // FIXED: inventoryOf is not declared anywhere; see gearEquip.
    Inventory& inv = invAt(s.game.party, idx);
    Item& it = inv.pack[pack];
    if (it.id <= 0 || !it.equipped) {
        reply = Str("Nothing worn in pack slot ") + gearNum(playerSlot) + ".\n";
        return;
    }
    const Str name = gearItemName(ctx, it);
    // UNKNOWN: no wording for a refused removal in the packet; the item is named and the
    // curse is stated.
    if (itemSlotCursed(it)) {
        reply = name + " is cursed and cannot be taken off.\n";
        return;
    }
    it.equipped = false;
    reply = Str("Member ") + gearNum(idx + 1) + " took off " + name + " from pack slot " + gearNum(playerSlot) + ". " + gearAdLine(ctx, c, inv) + "\n";
}

// Consumables and charged items. The usability rule (charges) comes from itemUseProblem
// in items3a.cpp. The spell effect is not wired (see applyItemUse), so no charge is spent.
static void gearUse(Ctx& ctx, Session& s, i32 idx, i32 playerSlot, Str& reply) {
    if (!gearSlotOk(playerSlot, reply)) return;
    const i32 pack = packIndexFromPlayerSlot(playerSlot);
    Character& c = s.game.party.members[static_cast<size_t>(idx)];
    // FIXED: inventoryOf is not declared anywhere; see gearEquip.
    Inventory& inv = invAt(s.game.party, idx);
    Item& it = inv.pack[pack];
    if (it.id <= 0) {
        reply = Str("Pack slot ") + gearNum(playerSlot) + " is empty.\n";
        return;
    }
    const Str name = gearItemName(ctx, it);
    const i32 problem = itemUseProblem(ctx, c, it);
    if (problem == static_cast<i32>(UseCheck::OutOfCharges)) {
        reply = name + " has no charges left.\n";
        return;
    }
    if (problem != static_cast<i32>(UseCheck::Ok)) {
        reply = name + " cannot be used now.\n";
        return;
    }
    Str effect;
    if (!applyItemUse(ctx, s, idx, pack, effect)) {
        reply = name + " was not used: its spell effect is not wired yet, so no charge was spent.\n";
        return;
    }
    reply = Str("Member ") + gearNum(idx + 1) + " used " + name + ". " + effect + " " + gearAdLine(ctx, c, inv) + "\n";
}

// Gear helpers. Rules come from items1b (equipProblem, equipItem), items3a (itemUseProblem)
// and attributes (totalAD, combatAD). Nothing here re-implements a rule.

// Decimal text for an i32.
static Str gearNum(i32 v) {
    if (v == 0) return Str("0");
    const bool neg = v < 0;
    i64 n = neg ? -static_cast<i64>(v) : static_cast<i64>(v);
    Str out;
    while (n > 0) {
        out.insert(out.begin(), static_cast<char>('0' + static_cast<int>(n % 10)));
        n /= 10;
    }
    return neg ? Str("-") + out : out;
}

// Item name from the static data. Named differently from the static itemNameOf stub
// and from items1b's itemNameOf, which would clash once those headers are included.
static Str gearItemName(const Ctx& ctx, const Item& it) {
    const ItemDef* def = findItemDef(ctx.data, it.id);
    return def != nullptr ? def->name : Str("?");
}

// Recomputed on every gear reply; nothing is cached.
static Str gearAdLine(const Ctx& ctx, const Character& c, const Inventory& inv) {
    const AdPair t = totalAD(ctx, c, inv);
    const i32 combat = combatAD(ctx, c, inv);
    return Str("Combat A/D ") + gearNum(combat) + ", total attack " + gearNum(t.att) + ", total defence " + gearNum(t.def) + ".";
}

// Pack slot 1 is the hands, not an item. The reference refuses item #1 the same way
// (ItemAnswer, FRMINTER.BAS:133-151, "item #1, the Hands, is slot 0 and so refused").
static bool gearSlotOk(i32 playerSlot, Str& reply) {
    if (playerSlot == kFirstPackSlot) {
        reply = "Slot 1 is your hands, not an item.\n";
        return false;
    }
    return true;
}

static void gearEquip(Ctx& ctx, Session& s, i32 idx, i32 playerSlot, Str& reply);
static void gearUnequip(Ctx& ctx, Session& s, i32 idx, i32 playerSlot, Str& reply);
static void gearUse(Ctx& ctx, Session& s, i32 idx, i32 playerSlot, Str& reply);

bool townAction(Session& s, const Str& command, Str& reply) {
    reply = Str();
    { bool fok = false; if (runFeatureCommand(s, Screen::Town, command, reply, fok)) return fok; }   // commands added by features
    const Vec<Str> w = splitWords(command);
    if (w.empty()) {
        reply = "No command given. Type help.\n";
        return false;
    }
    const Str verb = lowered(w[0]);
    Ctx ctx = s.ctx();
    Party& party = s.game.party;
    i32 idx = 0;
    i32 slot = 0;
    Guild guild = Guild::Nomad;

    if (verb == "help") {
        reply = kHelpText;
        return true;
    }
    if (verb == "rest") {
        doRest(ctx, s, reply);
        return true;
    }
    if (verb == "guild") {
        doGuildList(ctx, party, reply);
        return true;
    }
    if (verb == "levelup") {
        if (w.size() < 2 || !memberIndex(s, w[1], idx)) {
            return usage(ctx, reply, "levelup <member>");
        }
        doLevelUp(ctx, s, idx, reply);
        return true;
    }
    if (verb == "join") {
        if (w.size() < 3 || !memberIndex(s, w[1], idx) || !guildFromWord(w[2], guild)) {
            return usage(ctx, reply, "join <member> <guild>");
        }
        doJoin(ctx, s, idx, guild, reply);
        return true;
    }
    if (verb == "shop") {
        doShop(ctx, s, reply);
        return true;
    }
    if (verb == "buy") {
        i64 row = 0;
        if (w.size() < 3 || !parseNum(w[1], row) || !memberIndex(s, w[2], idx)) {
            return usage(ctx, reply, "buy <row> <member>");
        }
        doBuy(ctx, s, row, idx, reply);
        return true;
    }
    if (verb == "sell") {
        if (w.size() < 3 || !packSlot(w[1], slot) || !memberIndex(s, w[2], idx)) {
            return usage(ctx, reply, "sell <slot> <member>");
        }
        doSell(ctx, s, slot, idx, reply);  // FIXED: doSell takes Session&, not Party&; pass s.
        return true;
    }
    if (verb == "bank") {
        i64 gold = 0;
        if (w.size() < 3 || !parseNum(w[2], gold)) {
            return usage(ctx, reply, "bank deposit|withdraw <gold>");
        }
        const Str op = lowered(w[1]);
        if (op == "deposit") {
            doBank(ctx, party, true, gold, reply);
        } else if (op == "withdraw") {
            doBank(ctx, party, false, gold, reply);
        } else {
            return usage(ctx, reply, "bank deposit|withdraw <gold>");
        }
        return true;
    }
    if (verb == "deposit" || verb == "withdraw") {
        i64 gold = 0;
        if (w.size() < 2 || !parseNum(w[1], gold)) {
            // FIXED: usage takes const char*, so pick the literal form for this verb.
            return usage(ctx, reply, verb == "deposit" ? "deposit <gold>" : "withdraw <gold>");
        }
        doBank(ctx, party, verb == "deposit", gold, reply);
        return true;
    }
    if (verb == "equip" || verb == "unequip" || verb == "use") {
        if (w.size() < 3 || !memberIndex(s, w[1], idx) || !packSlot(w[2], slot)) {
            // Usage refusal: returns false and shows the command shape for this verb.
            return usage(ctx, reply,
                         verb == "equip"     ? "equip <member> <pack slot>"
                         : verb == "unequip" ? "unequip <member> <pack slot>"
                                             : "use <member> <pack slot>");
        }
        // Game refusals (empty slot, hands, guild, curse, charges) are replies that return
        // true, matching the buy/sell convention.
        if (verb == "equip") {
            gearEquip(ctx, s, idx, slot, reply);
        } else if (verb == "unequip") {
            gearUnequip(ctx, s, idx, slot, reply);
        } else {
            gearUse(ctx, s, idx, slot, reply);
        }
        return true;
    }
    if (verb == "identify") {
        if (w.size() == 1) {
            doIdentify(ctx, party, reply);
            return true;
        }
        if (w.size() < 3 || !packSlot(w[1], slot) || !memberIndex(s, w[2], idx)) {
            return usage(ctx, reply, "identify [<slot> <member>]");
        }
        doIdentifyItem(ctx, party, idx, slot, reply);
        return true;
    }
    if (verb == "morgue" || verb == "raise") {
        doMorgue(ctx, party, reply);
        return true;
    }
    if (verb == "spells") {
        // The spells a member can reach (listReachableSpellIds) with the level and the SP cost the casting module quotes.
        if (w.size() < 2 || !memberIndex(s, w[1], idx)) {
            return usage(ctx, reply, "spells <member>");
        }
        const Character& who = party.members[static_cast<size_t>(idx)];
        const Vec<i32> ids = listReachableSpellIds(ctx, who);
        if (ids.empty()) {
            note(ctx, reply, "town.spells", who.name + " can cast no spells yet.");
            return true;
        }
        note(ctx, reply, "town.spells", who.name + " can cast (" + toStr(who.vitals.curSpellPoints) + " of " + toStr(who.vitals.maxSpellPoints) + " spell points):");
        for (i32 id : ids) {
            for (const SpellDef& sp : s.data.spells) {
                if (sp.id != id) continue;
                note(ctx, reply, "town.spells", "  " + sp.name + "  level " + toStr(sp.level) + "  cost " + toStr(castCostSp(ctx, who, sp, 0)) + " SP");
                break;
            }
        }
        return true;
    }
    if (verb == "pool") {
        doPool(ctx, party, reply);
        return true;
    }
    if (verb == "descend") {
        doDescend(ctx, s, reply);
        return true;
    }
    reply = Str("Unknown town command: ") + command + "\n";
    return false;
}

// Identify one pack item. Validation is done here; the identify rule and the fee deduction
// are not in the packet (see NEED).
// identify <slot> <member>: identifies one pack item through the items module and reports
// the fee quoted and the gold the payer actually lost.
// R (ch13): identify one stored pack item for its owner. The fee is taken from the owner's
// purse by identifyItem (items2b_2.cpp), which also sets the identify level in the same step.
// townAction already calls this function, so it must exist for the file to build.
// R92 / ch13: identify one pack item for a fee. The fee is taken by identifyItem, which
// reports the outcome; the reply states what happened and what it cost.
// UNKNOWN: the R92 fee formula is not in the packet; identifyFee supplies it.
static void doIdentifyItem(Ctx& ctx, Party& party, i32 member, i32 slot, Str& reply) {
    if (member < 0 || member >= party.size) {
        note(ctx, reply, "town.identify", "No such party member.");
        return;
    }
    if (slot < kFirstPackSlot || slot > kLastPackSlot) {
        note(ctx, reply, "town.identify", "No such pack slot.");
        return;
    }
    Character& payer = memberAt(party, member);
    Item& item = invAt(party, member).pack[static_cast<u32>(slot)];
    // Quoted before identifyItem changes the item or takes the fee.
    const Currency fee = identifyFee(ctx, item);
    const Str who = payer.name;
    const Str where = "slot " + toStr(slot) + " of " + who;
    switch (identifyItem(ctx, payer, item)) {
        case IdentifyOutcome::Identified:
            note(ctx, reply, "town.identify",
                 "Identified the item in " + where + ". Fee paid: " +
                     toStr(fee / kGoldToCurrency) + " gold.");
            return;
        case IdentifyOutcome::NotNeeded:
            note(ctx, reply, "town.identify",
                 "The item in " + where + " is already fully identified. No fee was taken.");
            return;
        case IdentifyOutcome::CannotAfford:
            note(ctx, reply, "town.identify",
                 "Identify refused: " + who + " cannot pay the fee of " +
                     toStr(fee / kGoldToCurrency) + " gold. Nothing was changed.");
            return;
        case IdentifyOutcome::EmptySlot:
            note(ctx, reply, "town.identify", "Identify refused: " + where + " is empty.");
            return;
    }
}

// Morgue raise for one dead member. The cost, success rule and consequences are not in the
// packet (SRD ch12 "Morgue raise" and ch18 are headings only), so this validates and stops.
void doRaise(Ctx& ctx, const Party& party, i32 member, Str& reply) {
    if (member < 0 || member >= party.size) {
        note(ctx, reply, "town.morgue", "No such party member.");
        return;
    }
    const Character& ch = memberAt(party, member);
    if (!ch.isDead()) {
        note(ctx, reply, "town.morgue", ch.name + " is not dead.");
        return;
    }
    // UNKNOWN: the morgue fee, the success roll and the consequences of failure. The SRD text
    // was not supplied. NEED: the morgue raise rules and the raise call (death3.h / death1a.h
    // types are present but their functions are not).
    note(ctx, reply, "town.morgue", "Raising " + ch.name + " is not available yet.");
}

// Town spell helpers (file-local). Spell text is compared case-insensitively with spaces trimmed.
// FIXED: removed the duplicated copy of these helpers (townAllDigits, townKey, townJoin,
// townFindSpell, townFindById); each was defined twice, which is a redefinition error.
static bool townAllDigits(const Str& t) {
    if (t.empty()) return false;
    for (std::size_t i = 0; i < t.size(); ++i) {
        if (t[i] < '0' || t[i] > '9') return false;
    }
    return true;
}

static Str townKey(const Str& t) {
    std::size_t b = 0;
    std::size_t e = t.size();
    while (b < e && t[b] == ' ') ++b;
    while (e > b && t[e - 1] == ' ') --e;
    Str out;
    for (std::size_t i = b; i < e; ++i) {
        char ch = t[i];
        if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
        out += ch;
    }
    return out;
}

static Str townJoin(const Vec<Str>& w, std::size_t from, std::size_t to) {
    Str out;
    for (std::size_t i = from; i < to && i < w.size(); ++i) {
        if (!out.empty()) out += ' ';
        out += w[i];
    }
    return out;
}

// Name match first, then the numeric spell id. Returns nullptr when nothing matches.
static const SpellDef* townFindSpell(const GameData& data, const Str& text) {
    const Str want = townKey(text);
    if (want.empty()) return nullptr;
    for (std::size_t i = 0; i < data.spells.size(); ++i) {
        if (townKey(data.spells[i].name) == want) return &data.spells[i];
    }
    if (townAllDigits(want) && want.size() <= 9) {
        i32 id = 0;
        for (std::size_t i = 0; i < want.size(); ++i) id = id * 10 + static_cast<i32>(want[i] - '0');
        for (std::size_t i = 0; i < data.spells.size(); ++i) {
            if (data.spells[i].id == id) return &data.spells[i];
        }
    }
    return nullptr;
}

static const SpellDef* townFindById(const GameData& data, i32 id) {
    for (std::size_t i = 0; i < data.spells.size(); ++i) {
        if (data.spells[i].id == id) return &data.spells[i];
    }
    return nullptr;
}

// R4/R9: "spells <member>" lists the spells the member can reach, with the SP cost of each.
// Word vector w is splitWords(command); w[0] is "spells".
// R4/R9: a member word is the 1-based party position when it is all digits, otherwise a name.
static i32 townMemberOf(Session& s, const Str& word) {
    Party& party = s.game.party;
    if (townAllDigits(word)) {
        i32 n = 0;
        for (std::size_t i = 0; i < word.size() && n < 100000; ++i) {
            n = n * 10 + static_cast<i32>(word[i] - '0');
        }
        return (n >= 1 && n <= party.size) ? n - 1 : -1;
    }
    // FIXED: memberIndex takes the Session (not the Party) and an out-index; adapt the call.
    i32 idx = -1;
    if (!memberIndex(s, word, idx)) return -1;
    return idx;
}

// R4/R9: "spells <member>" lists the spells the member can reach, with the SP cost of each.
// w is splitWords(command); w[0] is "spells".
bool townSpells(Session& s, const Vec<Str>& w, Str& reply) {
    if (w.size() < 2) {
        reply = "Usage: spells <member>.";
        return false;
    }
    const i32 idx = townMemberOf(s, w[1]);
    if (idx < 0) {
        reply = "No such member.";
        return false;
    }
    Ctx ctx = s.ctx();
    Character& c = s.game.party.members[static_cast<std::size_t>(idx)];
    const Vec<i32> ids = listReachableSpellIds(ctx, c);
    if (ids.empty()) {
        reply = c.name + Str(" can reach no spells.");
        return true;
    }
    reply = Str("Spells ") + c.name + Str(" can reach (SP cost):");
    for (std::size_t k = 0; k < ids.size(); ++k) {
        const SpellDef* sp = townFindById(s.data, ids[k]);
        if (sp == nullptr) continue;
        const SpellUse use = spellUse(*sp);
        const i32 cost = castCostSp(ctx, c, *sp, 0);
        const char* tag = use == SpellUse::Combat ? " [combat]"
                        : (use == SpellUse::NonCombat ? " [town]" : "");
        reply += Str("\n  ") + sp->name + Str(" (#") + toStr(ids[k]) + Str(") ") + toStr(cost)
               + Str(" SP") + Str(tag);
    }
    return true;
}

// R4, R9: "cast <member> <spell name or id> [group]" outside combat.
// Refusals come before any SP is spent. The group number is accepted but has no meaning in town.
// R4, R5, R9: "cast <member> <spell name or id> [group]" outside combat.
// Refusals (unknown spell, unreachable spell, combat-only spell, too few SP) spend nothing.
// The group word is accepted and ignored: there are no monster groups in town.
bool townCast(Session& s, const Vec<Str>& w, Str& reply) {
    if (w.size() < 3) {
        reply = "Usage: cast <member> <spell name or id> [group].";
        return false;
    }
    const i32 who = townMemberOf(s, w[1]);
    if (who < 0) {
        reply = "No such member.";
        return false;
    }
    Party& party = s.game.party;
    Character& caster = party.members[static_cast<std::size_t>(who)];
    if (caster.isDead()) {
        reply = caster.name + Str(" is dead and cannot cast.");
        return false;
    }

    // Try the whole text first; then, with a trailing all-digit word, the text without it as the group.
    const Str full = townJoin(w, 2, w.size());
    const SpellDef* sp = townFindSpell(s.data, full);
    if (sp == nullptr && w.size() > 3 && townAllDigits(w[w.size() - 1])) {
        sp = townFindSpell(s.data, townJoin(w, 2, w.size() - 1));
    }
    if (sp == nullptr) {
        reply = Str("No spell called \"") + full + Str("\".");
        return false;
    }

    Ctx ctx = s.ctx();
    const Vec<i32> reach = listReachableSpellIds(ctx, caster);
    bool reachable = false;
    for (std::size_t k = 0; k < reach.size(); ++k) {
        if (reach[k] == sp->id) reachable = true;
    }
    if (!reachable) {
        reply = caster.name + Str(" cannot cast ") + sp->name + Str(".");
        return false;
    }
    if (spellUse(*sp) == SpellUse::Combat) {
        reply = sp->name + Str(" can only be cast in combat.");
        return false;
    }

    const i32 cost = castCostSp(ctx, caster, *sp, 0);
    if (caster.vitals.curSpellPoints < cost) {
        reply = caster.name + Str(" lacks the spell points for ") + sp->name
              + Str(" (needs ") + toStr(cost) + Str(").");
        return false;
    }
    payCastCost(caster, cost);

    // UNKNOWN: the non-combat effect (heal, protection, ailment cures) is not dispatched here yet;
    // the cost is spent and the reply says so. See NEED below.
    reply = caster.name + Str(" casts ") + sp->name + Str(" for ") + toStr(cost)
          + Str(" SP (") + toStr(caster.vitals.curSpellPoints) + Str(" left). The effect is not applied yet.");
    return true;
}

}  // namespace mordor
