#include <cstddef>

#include "mordor/game.h"
#include "mordor/features.h"
#include "mordor/combat1a.h"
#include "mordor/combat1b.h"
#include "mordor/combat2a.h"
#include "mordor/combat3.h"
#include "mordor/casting1.h"  // FIXED: declares castCostSp, payCastCost, listReachableSpellIds, spellUse, SpellUse
#include "mordor/monsters1a.h"
#include "mordor/monsters1b.h"
#include "mordor/progression1a.h"
#include "mordor/loot1a.h"
#include "mordor/clock.h"
#include "mordor/death1a.h"
#include "mordor/progression2.h"
#include "mordor/shared_helpers.h"

namespace mordor {
namespace {

// R1: combat tick 1000 ms. R7/R28: play clock 250 ms. R8: ailment tick 4000 ms.
constexpr i64 kCombatTickMs = 1000;
constexpr i64 kPlayTickMs = 250;
constexpr i64 kAilmentTickMs = 4000;

// Lower-case ASCII copy of a string (no locale, no clock, no I/O).
Str lowerStr(const Str& t) {
    Str out;
    for (std::size_t i = 0; i < t.size(); ++i) {
        char c = t[i];
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + 32);
        out += c;
    }
    return out;
}


}  // namespace
// Unsigned decimal parse; rejects anything that is not digits.
bool parseInt(const Str& t, i32& out) {
    if (t.size() == 0) return false;
    i64 v = 0;
    for (std::size_t i = 0; i < t.size(); ++i) {
        char c = t[i];
        if (c < '0' || c > '9') return false;
        v = v * 10 + (c - '0');
        if (v > 1000000) return false;
    }
    out = static_cast<i32>(v);
    return true;
}
namespace {


// Member token: 1-based number or character name (case-insensitive).
// Returns -1 when it does not name a member slot.
i32 memberIndex(const Party& p, const Str& t) {
    i32 v = 0;
    if (parseInt(t, v)) {
        v -= 1;
        return (v >= 0 && v < p.size) ? v : -1;
    }
    const Str want = lowerStr(t);
    for (i32 i = 0; i < p.size; ++i) {
        if (lowerStr(p.members[static_cast<std::size_t>(i)].name) == want) return i;
    }
    return -1;
}

// Optional aimed group token at position pos; 1-based on input, 0-based result, clamped.
i32 groupIndex(const Vec<Str>& tok, std::size_t pos) {
    i32 v = 1;
    if (pos < tok.size()) {
        if (!parseInt(tok[pos], v)) v = 1;
    }
    v -= 1;
    if (v < 0) v = 0;
    if (v > kMaxGroups - 1) v = kMaxGroups - 1;
    return v;
}

// Living foes: monsters still alive in the encounter.
i32 livingFoes(const GameState& g) {
    i32 n = 0;
    for (std::size_t i = 0; i < g.encounter.monsters.size(); ++i) {
        if (g.encounter.monsters[i].alive) ++n;
    }
    return n;
}

// True when every party slot is dead (or there is no party).
bool partyWiped(const Party& p) {
    for (i32 i = 0; i < p.size; ++i) {
        if (!p.members[static_cast<std::size_t>(i)].isDead()) return false;
    }
    return true;
}

// First living member index, or -1.
i32 firstLiving(const Party& p) {
    for (i32 i = 0; i < p.size; ++i) {
        if (!p.members[static_cast<std::size_t>(i)].isDead()) return i;
    }
    return -1;
}

// Standing order for every living member: Fight with an aimed group, or Defend.
// UNKNOWN: Character carries no CombatSetting value, so orders come from the command.
void setLivingOrders(Party& p, CombatOrderKind kind, i32 group) {
    for (i32 i = 0; i < p.size; ++i) {
        Character& c = p.members[static_cast<std::size_t>(i)];
        if (c.isDead()) continue;
        c.order.kind = kind;
        c.order.aimedGroup = group;
        c.order.spellId = 0;
    }
}

// Post-round settlement. Party wiped -> game over (onPartyWiped).
// No foes left -> endFight settles gold, XP and chests; endCombat clears the combat state.
// R11/R12: a member stays only while it is not dead (stoned counts as dead) and has hits.
static bool memberAble(const Character& c) {
    return !c.isDead() && c.vitals.curHits > 0;
}

// Listed member count, clamped to the array bounds.
static i32 listedCount(const Party& party) {
    if (party.size < 0) {
        return 0;
    }
    if (party.size > kPartyMaxSize) {
        return kPartyMaxSize;
    }
    return party.size;
}

// R11/R12: remove members that are not able, only while more than one is listed.
// Walks from the highest index down so a removal never shifts a member not yet visited.
// Name and location are read BEFORE removePartyMember, so nothing reads a shifted slot.
// The leader is never removed while it is the only listed member (R11).
static void pruneDeadMembers(Ctx& ctx, Party& party) {
    const i32 count = listedCount(party);
    if (count <= 1) {
        return;
    }
    // R12 step 2: the leader's level and square are the destination for survivors.
    const Location leaderSpot = party.members[0].location;
    const i32 leaderLevel = leaderSpot.dungeonLevel;
    for (i32 i = count - 1; i >= 0 && listedCount(party) > 1; --i) {
        Character& c = party.members[static_cast<u32>(i)];
        if (memberAble(c)) {
            continue;
        }
        const Str name = c.name;                                  // capture before any shift
        const bool announce = (c.location.dungeonLevel == leaderLevel);
        c.vitals.curHits = 0;
        if (!c.isDead()) {
            c.deathState = DeathState::Dead;
        }
        if (!removePartyMember(party, i)) {
            continue;
        }
        if (announce) {
            // R12 step 2: announced with the name first, from the captured copy.
            const Str line = name + " has died.";
            ctx.say("party.died", line, {}, {name});
        }
        // R12 step 2: remaining members move to the leader's level, X and Y.
        for (i32 j = 0; j < listedCount(party); ++j) {
            Character& m = party.members[static_cast<u32>(j)];
            m.location.dungeonLevel = leaderSpot.dungeonLevel;
            m.location.x = leaderSpot.x;
            m.location.y = leaderSpot.y;
        }
    }
}

// Keeps the commanded index on a living member (SRD SelectNextAble).
static void settleSelection(Party& party) {
    const i32 count = listedCount(party);
    if (count == 0) {
        party.selectedIndex = 0;
        return;
    }
    if (party.selectedIndex < 0 || party.selectedIndex >= count) {
        party.selectedIndex = 0;
    }
    if (memberAble(party.members[static_cast<u32>(party.selectedIndex)])) {
        return;
    }
    for (i32 step = 1; step <= count; ++step) {
        const i32 i = (party.selectedIndex + step) % count;
        if (memberAble(party.members[static_cast<u32>(i)])) {
            party.selectedIndex = i;
            return;
        }
    }
}

void settleRound(Ctx& ctx, GameState& g) {
    // Prune first (names captured before removal), then fix the selection.
    pruneDeadMembers(ctx, g.party);
    settleSelection(g.party);

    if (partyWiped(g.party)) {
        // R15: after pruning a wiped party holds one member, so onPartyWiped
        // no longer removes anyone. UNKNOWN: onPartyWiped body is not shown;
        // if it still removes members, its name reads must also be captured first.
        Arr<bool, kPartyMaxSize> counted{};
        onPartyWiped(ctx, g.party, counted);
        g.inCombat = false;
        return;
    }
    if (livingFoes(g) == 0) {
        endFight(ctx, g);
        endCombat(ctx, g);
        g.inCombat = false;
    }
}

}  // namespace

// R1, R5: the combat clock must be armed for a round to run. combatTick returns early
// when combatClockRunning is false, so the glue arms it when a fight starts and before every round.
static void armCombatClock(GameState& g) {
    if (g.inCombat) {
        g.combatClockRunning = true;
    }
}

// ---- local helpers (static, no mutable state) ----

// Monster table index for a creature, or -1.
static i32 monsterDefIndex(const Ctx& ctx, const Creature& m) {
    if (m.ref >= 0 && static_cast<std::size_t>(m.ref) < ctx.data.monsters.size()) {
        return m.ref;
    }
    return -1;
}

static Str monsterName(const Ctx& ctx, const Creature& m) {
    const i32 idx = monsterDefIndex(ctx, m);
    if (idx < 0) {
        return Str("a monster");
    }
    return ctx.data.monsters[static_cast<std::size_t>(idx)].name;
}

// Item table index matching an item, or -1.
static i32 itemDefIndex(const Ctx& ctx, const Item& it) {
    for (std::size_t i = 0; i < ctx.data.items.size(); ++i) {
        if (ctx.data.items[i].id == it.id) {  // UNKNOWN: Item.id matched to ItemDef.id
            return static_cast<i32>(i);
        }
    }
    return -1;
}

// Creatures of group gi occupy enc.monsters[start .. start+count). UNKNOWN: layout assumed.
static i32 groupStart(const Encounter& enc, i32 gi) {
    i32 start = 0;
    for (i32 j = 0; j < gi; ++j) {
        start += enc.groups[static_cast<std::size_t>(j)].count;
    }
    return start;
}

// Index into enc.monsters of the front living creature of group gi, or -1.
static i32 frontLiving(const Encounter& enc, i32 gi) {
    if (gi < 0 || gi >= kMaxGroups) {
        return -1;
    }
    const i32 start = groupStart(enc, gi);
    const i32 end = start + enc.groups[static_cast<std::size_t>(gi)].count;
    for (i32 i = start; i < end && static_cast<std::size_t>(i) < enc.monsters.size(); ++i) {
        if (enc.monsters[static_cast<std::size_t>(i)].hits > 0) {
            return i;
        }
    }
    return -1;
}

// Refreshes livingCount of group gi from the creatures' hits.
static void recountGroup(Encounter& enc, i32 gi) {
    const i32 start = groupStart(enc, gi);
    const i32 end = start + enc.groups[static_cast<std::size_t>(gi)].count;
    i32 n = 0;
    for (i32 i = start; i < end && static_cast<std::size_t>(i) < enc.monsters.size(); ++i) {
        if (enc.monsters[static_cast<std::size_t>(i)].hits > 0) {
            ++n;
        }
    }
    enc.groups[static_cast<std::size_t>(gi)].livingCount = n;
}

static i32 countLivingFoes(const Encounter& enc) {
    i32 n = 0;
    for (std::size_t i = 0; i < enc.monsters.size(); ++i) {
        if (enc.monsters[i].hits > 0) {
            ++n;
        }
    }
    return n;
}

static i32 raceSizeOf(const Ctx& ctx, const Character& c) {
    const i32 idx = static_cast<i32>(c.race);
    if (idx < 0 || static_cast<std::size_t>(idx) >= ctx.data.races.size()) {
        return 0;
    }
    return ctx.data.races[static_cast<std::size_t>(idx)].size;
}

static i32 monsterSizeOf(const Ctx& ctx, const Creature& m) {
    const i32 idx = monsterDefIndex(ctx, m);
    return idx < 0 ? 0 : ctx.data.monsters[static_cast<std::size_t>(idx)].size;
}

// Party phase for one member (R2, R6, R7, R8, R13, R22, R27, R28).
static void resolveMemberOrder(Ctx& ctx, GameState& g, i32 mi) {
    Party& party = g.party;
    Encounter& enc = g.encounter;  // UNKNOWN: GameState field name for the encounter
    const std::size_t mu = static_cast<std::size_t>(mi);
    Character& c = party.members[mu];
    const Inventory& inv = party.inventories[mu];

    if (c.isDead() || c.ailments.paralysed) {
        return;
    }
    if (c.order.kind == CombatOrderKind::Defend) {
        c.order.defendOn = true;  // R6: Defend suppresses the character's own attack
        return;
    }
    if (c.order.kind != CombatOrderKind::Fight) {
        return;  // Buffer (spell/use) orders are not resolved in this round. UNKNOWN: handled elsewhere
    }
    c.order.defendOn = false;

    const i32 fightGuild = chooseFightingGuild(ctx, c);  // R26. UNKNOWN: returns guild table index
    if (fightGuild < 0 || static_cast<std::size_t>(fightGuild) >= ctx.data.guilds.size()) {
        return;
    }
    const GuildDef& guild = ctx.data.guilds[static_cast<std::size_t>(fightGuild)];
    const i32 fightLevel = c.guilds[static_cast<std::size_t>(fightGuild)].level;
    const f64 fightingMod = guild.abilities[0] / 10.0;  // R24. UNKNOWN: fighting ability index
    const i32 strength = c.abilities.stats[static_cast<std::size_t>(StatIndex::Str)];
    const i32 attack = attackOf(ctx, c, inv);

    bool anySwing = false;
    const Vec<i32> slots = equippedWeaponSlots(inv);  // UNKNOWN: slot indexes inv.worn
    for (std::size_t si = 0; si < slots.size(); ++si) {
        const i32 slot = slots[si];
        if (slot < 0 || static_cast<std::size_t>(slot) >= inv.worn.size()) {
            continue;
        }
        const Item& w = inv.worn[static_cast<std::size_t>(slot)];
        const i32 wIdx = itemDefIndex(ctx, w);
        const i32 weaponSwings = wIdx >= 0 ? ctx.data.items[static_cast<std::size_t>(wIdx)].swings : 0;
        const i32 weaponFloor = wIdx >= 0 ? ctx.data.items[static_cast<std::size_t>(wIdx)].floor : 0;
        const f64 weaponMod = wIdx >= 0 ? ctx.data.items[static_cast<std::size_t>(wIdx)].damageMod : 0.0;

        const i32 swings = swingsPerWeapon(c.combat.swings, weaponSwings);  // R7
        if (swings < 1) {
            continue;
        }
        anySwing = true;

        i32 left = swings;
        bool killedLast = false;
        bool going = true;
        while (going && (left > 0 || killedLast)) {  // R27: a kill continues the chain
            if (left > 0) {
                --left;
            }
            // R3, R5: keep the aimed group while it stands, otherwise move aim
            const i32 aimed = furtherSwingAim(enc, c.order.aimedGroup);
            c.order.aimedGroup = aimed;
            const i32 ti = frontLiving(enc, aimed);
            if (ti < 0) {
                break;
            }
            Creature& m = enc.monsters[static_cast<std::size_t>(ti)];

            const WeaponGateOutcome gate = checkWeaponGate(m, weaponFloor);
            if (static_cast<i32>(gate) != 0) {  // UNKNOWN: pass outcome assumed to be value 0
                // R13: NeedsGoodWeapon (bit 4) switches the actor to Defend and clears the item order
                if ((m.specialPropertyFlags & (1u << 4)) != 0 && weaponFloor < m.levelFound / 2) {
                    c.order.kind = CombatOrderKind::Defend;
                    party.itemOrders[mu] = ItemOrder{};
                }
                ctx.say("combat.gate", c.name + Str(" cannot harm ") + monsterName(ctx, m) + Str(" with this weapon."), {}, {});
                break;
            }

            const f64 critPct = critChance(guild.criticalHit, 0, m.guildLevel, weaponFloor, 0);  // UNKNOWN: critMod, weaponCrit
            const f64 backPct = backstabChance(guild.backstab, 0, m.guildLevel, weaponFloor, 0);  // UNKNOWN: backMod, weaponBackstab
            const BlowProcs procs = rollBlowProcs(ctx, critPct, backPct);
            const i32 bonus = blowModifierBonus(procs);
            // UNKNOWN: flags decoded from bonus assuming crit = +5 and backstab = +2 (R23, R24)
            const bool crit = (bonus == 5 || bonus == 7);
            const bool backstab = (bonus == 2 || bonus == 7);
            const bool unseen = m.unseen && !c.ailments.seeInvisible;  // R16

            const i32 sizeDiff = raceSizeOf(ctx, c) - monsterSizeOf(ctx, m);
            const i32 dmg = characterDamage(ctx, fightLevel, fightingMod, weaponMod, strength,
                                            sizeDiff, crit, backstab, attack, unseen);

            if (dmg < 1) {  // R16, R22: a miss ends the weapon's swings
                ctx.say("combat.miss", c.name + Str(" misses ") + monsterName(ctx, m) + Str("."), {}, {});
                break;
            }

            const i32 recorded = recordedHitsFor(dmg, m.hits);  // R31: overkill not counted
            m.hits -= recorded;
            c.damageRecorded += recorded;
            ctx.say("combat.hit", c.name + Str(" hits ") + monsterName(ctx, m) + Str(" for ") + toStr(recorded) + Str("."),
                    {recorded}, {});

            if (recorded > 0) {
                // UNKNOWN: experience per hit is not specified; level of the monster used as the rate
                giveExperience(ctx, c, c.activeGuild, static_cast<i64>(recorded) * static_cast<i64>(m.levelFound));  // XP goes to the active guild membership (SRD ch.5)
            }

            const bool killed = (m.hits <= 0);
            if (killed) {
                m.hits = 0;
                m.alive = false;
                noteMonsterKill(ctx, c, m);
                ctx.say("combat.kill", c.name + Str(" kills ") + monsterName(ctx, m) + Str("."), {}, {});
                recountGroup(enc, aimed);
            }
            killedLast = killed;
            going = weaponChainContinues(dmg, killed);
        }
    }

    if (!anySwing) {
        // R8: no usable weapon or swings puts the actor on Defend
        ctx.say("combat.noweapon", c.name + Str(" has no usable weapon: using no weapon! and defends."), {}, {});
        c.order.kind = CombatOrderKind::Defend;
        c.order.defendOn = true;
    }
}

// Monster phase (R1, R28): each group in order; each living creature attacks one living member.
static void resolveMonsterPhase(Ctx& ctx, GameState& g) {
    Party& party = g.party;
    Encounter& enc = g.encounter;  // UNKNOWN: GameState field name for the encounter
    // FIXED: GameState has no companions member; use an empty local roster instead.
    Arr<Companion, kCompanionSlots> companions{};

    for (i32 gi = 0; gi < kMaxGroups; ++gi) {
        const i32 start = groupStart(enc, gi);
        const i32 end = start + enc.groups[static_cast<std::size_t>(gi)].count;
        for (i32 i = start; i < end && static_cast<std::size_t>(i) < enc.monsters.size(); ++i) {
            Creature& m = enc.monsters[static_cast<std::size_t>(i)];
            if (m.hits <= 0) {
                continue;
            }
            const i32 ti = pickMonsterTarget(ctx, party, companions);
            if (ti < 0) {
                return;  // no living member left
            }
            const std::size_t tu = static_cast<std::size_t>(ti);
            Character& v = party.members[tu];
            const Inventory& vinv = party.inventories[tu];
            const i32 def = applyDefendBonus(defenseOf(ctx, v, vinv), v.order.defendOn);

            if (!monsterHitRoll(ctx, m.att, def)) {
                ctx.say("combat.monstermiss", monsterName(ctx, m) + Str(" misses ") + v.name + Str("."), {}, {});
                continue;
            }
            const i32 mIdx = monsterDefIndex(ctx, m);
            const f64 damMod = mIdx >= 0 ? ctx.data.monsters[static_cast<std::size_t>(mIdx)].damageMod : 0.0;
            const i32 dmg = monsterBlow(ctx, m.att, def, m.guildLevel, m.str, damMod);
            if (dmg < 1) {
                ctx.say("combat.monstermiss", monsterName(ctx, m) + Str(" fails to hurt ") + v.name + Str("."), {}, {});
                continue;
            }
            v.vitals.curHits -= dmg;
            ctx.say("combat.monsterhit", monsterName(ctx, m) + Str(" hits ") + v.name + Str(" for ") + toStr(dmg) + Str("."),
                    {dmg}, {});
            if (v.vitals.curHits <= 0) {
                v.vitals.curHits = 0;
                v.deathState = DeathState::Dead;
                processMidRoundDeath(ctx, party, ti);  // R28
                ctx.say("combat.death", v.name + Str(" has died."), {}, {});
            }
        }
    }
}


static Str castKey(const Str& t) {
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

static Str castJoin(const Vec<Str>& w, std::size_t from, std::size_t to) {
    Str out;
    for (std::size_t i = from; i < to && i < w.size(); ++i) {
        if (!out.empty()) out += ' ';
        out += w[i];
    }
    return out;
}

// Name match first (case-insensitive), then the numeric spell id.
static const SpellDef* castFindSpell(const GameData& data, const Str& text) {
    const Str want = castKey(text);
    if (want.empty()) return nullptr;
    for (std::size_t i = 0; i < data.spells.size(); ++i) {
        if (castKey(data.spells[i].name) == want) return &data.spells[i];
    }
    if (isDigits(want) && want.size() <= 9) {
        i32 id = 0;
        for (std::size_t i = 0; i < want.size(); ++i) id = id * 10 + static_cast<i32>(want[i] - '0');
        for (std::size_t i = 0; i < data.spells.size(); ++i) {
            if (data.spells[i].id == id) return &data.spells[i];
        }
    }
    return nullptr;
}

// R2/R4: a spell order resolves in the party phase. The SP are spent here (not when ordered).
// A member whose order has no spell id (a pack-item Use order) is left to the caller. UNKNOWN: spell id 0 means no spell.
static void resolveSpellOrder(Ctx& ctx, GameState& g, i32 mi) {
    Party& party = g.party;
    Character& c = party.members[static_cast<std::size_t>(mi)];
    if (c.isDead() || c.ailments.paralysed) {
        return;
    }
    const SpellDef* sp = nullptr;
    if (c.order.spellId != 0) {
        for (std::size_t i = 0; i < ctx.data.spells.size(); ++i) {
            if (ctx.data.spells[i].id == c.order.spellId) {
                sp = &ctx.data.spells[i];
                break;
            }
        }
    }
    if (sp == nullptr) {
        return;
    }
    const i32 cost = castCostSp(ctx, c, *sp, 0);
    if (c.vitals.curSpellPoints < cost) {
        // UNKNOWN: the reference refuses at cast time; here the order fails and the member does nothing else this round.
        ctx.say("magic.nosp", c.name + Str(" lacks the spell points for ") + sp->name + Str("."), {cost}, {});
    } else {
        payCastCost(c, cost);
        ctx.say("magic.cast", c.name + Str(" casts ") + sp->name + Str("."), {cost}, {});
        // NEED: the combat effect of the spell (damage via spellDamage, heal, buffs) is not applied yet.
    }
    c.order.spellId = 0;
    c.order.kind = CombatOrderKind::Fight;  // the next command resets all orders; no attack this round
}

bool combatAction(Session& s, const Str& command, Str& reply) {
    Ctx ctx = s.ctx();
    GameState& g = s.game;
    Party& party = g.party;
    const std::size_t mark = s.log.events.size();
    reply = Str();
    { bool fok = false; if (runFeatureCommand(s, Screen::Combat, command, reply, fok)) return fok; }   // commands added by features

    Vec<Str> tok = splitWords(command);
    if (tok.size() == 0) {
        reply = "Enter a combat command: fight, defend, cast, use or open.";
        return false;
    }
    const Str verb = lowerStr(tok[0]);

    if (verb == "fight") {
        const i32 group = groupIndex(tok, 1);
        if (!g.inCombat) {
            if (livingFoes(g) == 0) {
                reply = "There is nothing to fight.";
                return false;
            }
            startFight(ctx, g);
            g.inCombat = true;
            armCombatClock(g);  // R1, P1: the fight starts the combat clock
        }
        setLivingOrders(party, CombatOrderKind::Fight, group);
    } else if (verb == "defend") {
        if (!g.inCombat) {
            reply = "You are not in combat.";
            return false;
        }
        setLivingOrders(party, CombatOrderKind::Defend, 0);
    } else if (verb == "cast" || verb == "use") {
        if (!g.inCombat) {
            reply = "You are not in combat.";
            return false;
        }
        if (tok.size() < 3) {
            reply = "Usage: cast <member> <spell name or id> [group]  or  use <member> <slot>.";
            return false;
        }
        const i32 who = memberIndex(party, tok[1]);
        if (who < 0) {
            reply = "No such member.";
            return false;
        }
        if (party.members[static_cast<std::size_t>(who)].isDead()) {
            reply = "That member is dead.";
            return false;
        }
        const std::size_t wi = static_cast<std::size_t>(who);

        if (verb == "cast") {
            // Spell name or id; a trailing all-digit word after at least one text word is the group.
            std::size_t textEnd = tok.size();
            if (textEnd - 2 > 1 && isDigits(tok[textEnd - 1])) --textEnd;
            const Str text = castJoin(tok, 2, textEnd);
            const SpellDef* sp = castFindSpell(s.data, text);
            if (sp == nullptr) {
                reply = Str("No spell called \"") + text + Str("\".");
                return false;
            }
            Character& caster = party.members[wi];
            const Vec<i32> reach = listReachableSpellIds(ctx, caster);
            bool reachable = false;
            for (std::size_t k = 0; k < reach.size(); ++k) {
                if (reach[k] == sp->id) reachable = true;
            }
            if (!reachable) {
                reply = caster.name + Str(" cannot cast ") + sp->name + Str(".");
                return false;
            }
            if (spellUse(*sp) == SpellUse::NonCombat) {
                reply = sp->name + Str(" can only be cast outside combat.");
                return false;
            }
            const i32 cost = castCostSp(ctx, caster, *sp, 0);
            if (caster.vitals.curSpellPoints < cost) {
                reply = caster.name + Str(" lacks the spell points for ") + sp->name + Str(" (needs ") + toStr(cost) + Str(").");
                return false;
            }
            const i32 group = textEnd < tok.size() ? groupIndex(tok, textEnd) : 0;
            // Other members keep Fight; the caster's order is the spell, paid when the order resolves.
            setLivingOrders(party, CombatOrderKind::Fight, group);
            caster.order.kind = CombatOrderKind::Buffer;
            caster.order.spellId = sp->id;
        } else {
            i32 arg = 0;
            if (!parseInt(tok[2], arg)) {
                reply = "No such member or number.";
                return false;
            }
            // SIMPLIFIED: pack-item Use is handled by the caller; the slot is kept for it.
            setLivingOrders(party, CombatOrderKind::Fight, 0);
            Character& c = party.members[wi];
            c.order.kind = CombatOrderKind::Buffer;
            c.order.spellId = 0;
            c.order.spellBuffers[0] = arg;
        }
    } else if (verb == "open") {
        if (!g.inCombat) {
            reply = "You are not in combat.";
            return false;
        }
        const i32 opener = firstLiving(party);
        if (opener < 0) {
            reply = "No living member can open a chest.";
            return false;
        }
        // SIMPLIFIED: the chest is opened by the first living member; the square is not located here.
        party.opener = opener;
        setLivingOrders(party, CombatOrderKind::Fight, 0);
    } else {
        reply = "Unknown combat command.";
        return false;
    }

    // One full combat round, composed from the combat modules. combatTick is not used:
    // it only emits ordering and attack events and never resolves hits, deaths or the end of the fight.
    if (g.inCombat) {
        Encounter& enc = g.encounter;  // UNKNOWN: GameState field name for the encounter

        // Party phase: each living member in order (R2). Spell orders resolve here too.
        for (i32 mi = 0; mi < party.size; ++mi) {
            if (party.members[static_cast<std::size_t>(mi)].order.kind == CombatOrderKind::Buffer) {
                resolveSpellOrder(ctx, g, mi);
            } else {
                resolveMemberOrder(ctx, g, mi);
            }
        }

        // Companions act for each living member, if they are present.
        for (i32 mi = 0; mi < party.size; ++mi) {
            Character& owner = party.members[static_cast<std::size_t>(mi)];
            if (!owner.isDead()) {
                companionsAct(ctx, g, owner);
            }
        }

        // Monster phase: every group attacks in order (R1, R28).
        resolveMonsterPhase(ctx, g);

        // Fight end: no creature left (R1 end-of-fight handling).
        if (countLivingFoes(enc) == 0) {
            endFight(ctx, g);
            endCombat(ctx, g);
            g.inCombat = false;
            payFightGold(ctx, party);   // fight gold split among the party (SRD ch.14 R5)
            // UNKNOWN: markRoomCleared needs a FloorState, room index and nowMs. None are
            // reachable from combatAction (no time argument, no floor handle), so it is left to the caller.
        }
    }
    if (g.inCombat || partyWiped(party)) settleRound(ctx, g);

    // Reply: events produced since this call began, then a one-line summary.
    // UNKNOWN: EventLog::events element field `text` assumed per spec.
    for (std::size_t i = mark; i < s.log.events.size(); ++i) {
        if (reply.size() > 0) reply += '\n';
        reply += s.log.events[i].text;
    }
    Str summary = g.inCombat ? Str("Round fought: ") + toStr(livingFoes(g)) + Str(" foe(s) left.")
                             : Str("Fight over.");
    if (reply.size() > 0) reply += '\n';
    reply += summary;
    return true;
}

// R1, R5: the combat clock is armed while a fight is on, so every tick it runs a round.
// Minimising and underground remain gates inside combatTick.
void advanceTime(Session& s, i64 ms) {
    if (ms <= 0) return;
    Ctx ctx = s.ctx();
    GameState& g = s.game;

    // Combat clock (R1): g.lastTickMs holds the absolute combat-clock time, so the
    // remainder is kept implicitly by the modulus. SIMPLIFIED: reuse of this field.
    const i64 oldC = g.lastTickMs;
    g.lastTickMs += ms;
    const i64 combatTicks = g.lastTickMs / kCombatTickMs - oldC / kCombatTickMs;
    if (g.inCombat) {
        g.combatClockRunning = true;  // R1, P1: precondition for combatTick before every round
        for (i64 k = 0; k < combatTicks && g.inCombat; ++k) {
            combatTick(ctx, g);
            settleRound(ctx, g);
        }
    }

    // Play clock (R7, R28): g.playElapsedMs holds the absolute play-clock time.
    // SIMPLIFIED: one playClockTick call per advance that crosses a 250 ms boundary.
    const i64 oldP = g.playElapsedMs;
    g.playElapsedMs += ms;
    if (g.playClockRunning && !g.paused && !g.minimised &&
        g.playElapsedMs / kPlayTickMs > oldP / kPlayTickMs) {
        playClockTick(g, g.playElapsedMs);
    }

    // Ailment clock (R8): g.ailmentElapsedMs is the remainder accumulator.
    g.ailmentElapsedMs += ms;
    const i64 ailmentTicks = g.ailmentElapsedMs / kAilmentTickMs;
    g.ailmentElapsedMs -= ailmentTicks * kAilmentTickMs;
    for (i64 k = 0; k < ailmentTicks; ++k) {
        ailmentTick(ctx, g);
    }
}

}  // namespace mordor
