// partyb.cpp
// Party roster rules: join, leave, pooled gold, selection, combat settings,
// autoload, city join, bodies in the city, give-gold, and monster target choice.
// SRD references (c10..c59) are the source-note rule numbers from the engine notes.
#include "mordor/partyb.h"

#include "mordor/math_util.h"
#include "mordor/rng.h"

// HEADER BUG (partyb.h): the declaration
//     GoldTransferOutcome transferGold(Ctx&, Character&, Character&, Currency, bool);
// uses a type that is not declared in partyb.h, the state headers, types.h, ctx.h
// or the API index. A function with an undeclared return type cannot be defined, so
// transferGold is NOT defined in this file. Once the type is supplied (or the header
// is corrected), add the definition using the SRD logic below (c16, Exchange/transfer):
//     if amount == 0: return
//     available = inDungeon ? from.onhand : (from.onhand + from.bank)
//     if amount > available: refuse "Too Much!"
//     to.onhand += amount; from.onhand -= amount
//     if from.onhand < 0: from.bank += from.onhand; from.onhand = 0
// No negative-amount check is in the SRD text, so a negative amount runs backwards
// (documented original quirk, c16).
//
// NOTE: giveItemToMember takes no Inventory, so the empty-slot and Equipped checks
// cannot be made here; they belong to the items module.

namespace mordor {

namespace {

constexpr i64 kCurrencyMax = 9223372036854775807LL;
constexpr i64 kCurrencyMin = -9223372036854775807LL - 1;

// Overflow-checked Currency add. Returns false if the sum would overflow i64.
bool addCurrencyChecked(Currency a, Currency b, Currency& out) {
    if (b > 0 && a > kCurrencyMax - b) return false;
    if (b < 0 && a < kCurrencyMin - b) return false;
    out = a + b;
    return true;
}

// Index of the first member with this name, or -1.
i32 findMemberIndex(const Party& party, const Str& name) {
    for (i32 i = 0; i < party.size; ++i) {
        if (party.members[i].name == name) return i;
    }
    return -1;
}

// Keeps SelectedIndex inside [0, Count-1].
void clampSelected(Party& party) {
    if (party.size <= 0) {
        party.selectedIndex = 0;
        return;
    }
    if (party.selectedIndex < 0) party.selectedIndex = 0;
    if (party.selectedIndex > party.size - 1) party.selectedIndex = party.size - 1;
}

// Select(slot): validates the slot and sets SelectedIndex.
bool selectSlot(Party& party, i32 slot) {
    if (slot < 0 || slot >= party.size) return false;
    party.selectedIndex = slot;
    return true;
}

// Adds a copy of c at the end of the party (no-op when full).
void appendMember(Party& party, const Character& c) {
    if (party.size >= kPartyMaxSize) return;
    party.members[party.size] = c;
    party.size += 1;
}

// Removes the member at index (shifts later members left). Does not touch SelectedIndex.
void removeAt(Party& party, i32 index) {
    for (i32 j = index; j + 1 < party.size; ++j) {
        party.members[j] = party.members[j + 1];
    }
    party.size -= 1;
    party.members[party.size] = Character{};
}

// UNKNOWN: engine SameArea maps level 0 to area -1. The square fallback for old saves
// (Party.cs) and the audit's X/Y claim are not reproduced; see CONFLICT in the notes.
i32 savedArea(const Character& c) {
    return c.location.dungeonLevel == 0 ? -1 : c.location.area;
}

// UNKNOWN: ApplyDefault body is not in the source chunks. Simplest sensible rule:
// a None order becomes Fight; the Target is kept (c54 default setting 1 = Fight).
void applyDefaultOrder(Character& c) {
    if (c.order.kind == CombatOrderKind::None) {
        c.order.kind = CombatOrderKind::Fight;
    }
}

// PruneTheDead (c16 UPDATE/PruneTheDead). Removes dead or non-able members while more
// than one is left. The index does not advance after a removal (original quirk, c16).
void pruneTheDead(Ctx& ctx, Party& party) {
    if (party.size <= 1) return;

    const i32 leaderLevel = party.members[0].location.dungeonLevel;
    i32 i = 0;
    while (i < party.size && party.size > 1) {
        Character& m = party.members[i];
        const bool able = !m.isDead() && m.vitals.curHits > 0 &&
                          m.deathState != DeathState::Stoned;
        if (able) {
            i += 1;
            continue;
        }

        m.vitals.curHits = 0;
        if (m.location.dungeonLevel == leaderLevel) {
            ctx.say("party.member.died", "\"<name>\" has died!", {}, {m.name});
        }
        if (!m.isDead()) {
            m.deathState = DeathState::Dead;
        }
        // save(member): writing the character file is the caller's job; the core
        // has no file access.

        const bool removedBeforeSelected = i < party.selectedIndex;
        removeAt(party, i);
        // UNKNOWN: the SRD text reads "SelectedIndex--? if i < SelectedIndex".
        // Simplest sensible rule: decrement when the removed slot was before the selection.
        if (removedBeforeSelected) party.selectedIndex -= 1;
        clampSelected(party);

        // All remaining members move to the current leader's level, X and Y.
        const Location lead = party.members[0].location;
        for (i32 k = 0; k < party.size; ++k) {
            party.members[k].location.dungeonLevel = lead.dungeonLevel;
            party.members[k].location.x = lead.x;
            party.members[k].location.y = lead.y;
        }
        // Index i is not advanced (original quirk, c16).
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// Help cry (c11)
// ---------------------------------------------------------------------------

bool helpCryNeeded(const Character& c) {
    // Needed = CurHits < 10 OR CurHits <= MaxHits * 0.1
    const i32 cur = c.vitals.curHits;
    if (cur < kHelpCryHitsAbsolute) return true;
    return static_cast<double>(cur) <=
           static_cast<double>(c.vitals.maxHits) * kHelpCryHitsFraction;
}

// ---------------------------------------------------------------------------
// Monster target choice (c14)
// ---------------------------------------------------------------------------

i32 monsterTargetWeight(i32 position) {
    // Table is integer-halving: 100,50,25,12,6,3,1,0. Positions past the table
    // (and the last entry, 0) weigh 0.
    if (position < 0) return 0;
    const i32 last = kMonsterTargetWeightCount - 1;
    if (position >= kMonsterTargetWeightCount) return kMonsterTargetWeights[last];
    return kMonsterTargetWeights[position];
}

i32 monsterTargetTotal(i32 partySize) {
    // total = sum of the weight for each party position
    i32 total = 0;
    for (i32 i = 0; i < partySize; ++i) {
        total += monsterTargetWeight(i);
    }
    return total;
}

i32 pickMonsterTarget(Ctx& ctx, i32 partySize) {
    // total = sum of weights; roll = Rnd(100) + 1
    // target = 0; acc = 100; step = 100
    // while (acc / total * 100 < roll): target++; step /= 2; acc += step
    // target = min(target, last)
    const i32 total = monsterTargetTotal(partySize);
    if (total <= 0) return 0;  // UNKNOWN: empty party; not reachable from the SRD text.

    const i32 roll = static_cast<i32>(ctx.rng.below(kMonsterTargetRollSides)) + 1;

    i32 target = 0;
    i32 acc = 100;
    i32 step = 100;
    // UNKNOWN: "acc / total * 100" is taken as floating point. Integer division
    // would give 0 and never terminate. The bound on target is a safety guard only;
    // the accumulated total can never reach the roll before position 8.
    while (static_cast<double>(acc) / static_cast<double>(total) * 100.0 <
               static_cast<double>(roll) &&
           target < kMonsterTargetWeightCount) {
        target += 1;
        step /= 2;
        acc += step;
    }

    const i32 last = kMonsterTargetWeightCount - 1;
    if (target > last) target = last;
    return target;
}

// ---------------------------------------------------------------------------
// Joining and alignment (c16)
// ---------------------------------------------------------------------------

PartyJoinOutcome joinParty(Ctx& ctx, Party& party, Character& newcomer, bool inCombat) {
    // if Members.Count == 0: join
    if (party.size == 0) {
        appendMember(party, newcomer);
        return PartyJoinOutcome::Joined;
    }
    // UNKNOWN: message for a dead first member (SRD "Chars(0).DeathState: refuse").
    if (party.members[0].isDead()) {
        return PartyJoinOutcome::LeaderDead;
    }
    // if Members.Count >= 4: refuse "Your party is too full!"
    if (party.size >= kPartyMaxSize) {
        ctx.say("party.join.full", "Your party is too full!", {}, {});
        return PartyJoinOutcome::Full;
    }
    // if any member.Name == c.Name: refuse AlreadyInParty
    if (findMemberIndex(party, newcomer.name) >= 0) {
        ctx.say("party.join.already", "Character is already in Party!", {}, {});
        return PartyJoinOutcome::AlreadyInParty;
    }
    // Combat check (c59): "if fn0EEF() > 0 and joiner dead: Cannot raise character in Combat!"
    if (inCombat && newcomer.isDead()) {
        ctx.say("party.join.deadInCombat", "Cannot raise character in Combat!", {}, {});
        return PartyJoinOutcome::DeadInCombat;
    }
    // if c.IsDead: refuse IsDead
    if (newcomer.isDead()) {
        ctx.say("party.join.dead", "Character is Dead!", {}, {});
        return PartyJoinOutcome::Dead;
    }
    // if not SamePlace(Leader, c): refuse NotHere
    if (!samePlace(party.leader(), newcomer)) {
        ctx.say("party.join.notHere", "Character is not here!", {}, {});  // UNKNOWN: exact box
        return PartyJoinOutcome::NotHere;
    }
    // if any Opposed(member.Alignment, c.Alignment): refuse oppositely aligned
    for (i32 i = 0; i < party.size; ++i) {
        if (opposedAlignment(party.members[i].alignment, newcomer.alignment)) {
            ctx.say("party.join.opposed",
                    "\"<name>\" is oppositely aligned to one or more members of the party!",
                    {}, {newcomer.name});
            return PartyJoinOutcome::Opposed;
        }
    }
    // Mid-fight setup for living joiners only (c59, sub0BDE).
    if (inCombat) {
        setupJoinerInCombat(ctx, party, newcomer);
    }
    // sub0848 True: newcomer lost flags = Chars(0) lost flags (c16)
    inheritLostFlags(ctx, party.members[0], newcomer);
    appendMember(party, newcomer);
    return PartyJoinOutcome::Joined;
}

bool opposedAlignment(Align a, Align b) {
    // Opposed(a, b) = (a==Good and b==Evil) or (a==Evil and b==Good)
    return (a == Align::Good && b == Align::Evil) || (a == Align::Evil && b == Align::Good);
}

bool samePlace(const Character& a, const Character& b) {
    // (a.DungeonLevel==0 and b.DungeonLevel==0) or
    // (same level and same X and Y)
    if (a.location.dungeonLevel == 0 && b.location.dungeonLevel == 0) return true;
    return a.location.dungeonLevel == b.location.dungeonLevel &&
           a.location.x == b.location.x && a.location.y == b.location.y;
}

Vec<i32> hereToJoinCandidates(const Party& party,
                              const Vec<Character>& roster,
                              i32 partyLevel,
                              i32 floorCount,
                              i32 area) {
    // HereToJoin = everyone where not in party
    //              and DungeonLevel == level
    //              and 1 <= DungeonLevel <= floors
    //              and savedArea(c) == area
    Vec<i32> out;
    if (partyLevel < 1 || partyLevel > floorCount) return out;
    const i32 count = static_cast<i32>(roster.size());
    for (i32 r = 0; r < count; ++r) {
        const Character& c = roster[r];
        if (findMemberIndex(party, c.name) >= 0) continue;
        if (c.location.dungeonLevel != partyLevel) continue;
        if (savedArea(c) != area) continue;
        out.push_back(r);
    }
    return out;
}

void inheritLostFlags(Ctx& ctx, const Character& first, Character& newcomer) {
    // newcomer.LostLevel = first.LostLevel; LostPosition; LostFacing (c16)
    static_cast<void>(ctx);
    newcomer.lost.level = first.lost.level;
    newcomer.lost.position = first.lost.position;
    newcomer.lost.facing = first.lost.facing;
}

// ---------------------------------------------------------------------------
// Pooling gold (c16, c43, c48)
// ---------------------------------------------------------------------------

bool poolGoldToSelected(Ctx& ctx, Party& party) {
    (void)ctx;
    // R24: pool each other member's on-hand gold, in index order, into the selected member.
    // Stop at the first member whose full amount would overflow the selected member's cap;
    // that member and all later members keep their gold.
    // UNKNOWN: SRD does not say whether a partial amount is moved before stopping; whole-member only.
    // FIXED: memberAt is not declared anywhere; use direct member indexing.
    Character& sel = party.members[party.selectedIndex];
    for (i32 i = 0; i < party.size; ++i) {
        if (i == party.selectedIndex) continue;
        Character& m = party.members[i];
        const Currency g = m.wallet.goldOnHand;
        if (g <= 0) continue;
        // FIXED: kBalanceCap is not declared anywhere; use the local kCurrencyMax cap.
        if (g > kCurrencyMax - sel.wallet.goldOnHand) {
            return false;  // overflow: stop pooling
        }
        sel.wallet.goldOnHand += g;
        m.wallet.goldOnHand -= g;
    }
    return true;
}

bool poolGoldToBank(Ctx& ctx, Party& party) {
    // PoolToBank: to.GoldInBank += m.GoldOnHand; m.GoldOnHand = 0 (all members, to included)
    static_cast<void>(ctx);
    const i32 to = party.selectedIndex;
    if (to < 0 || to >= party.size) return true;
    Character& dest = party.members[to];
    for (i32 i = 0; i < party.size; ++i) {
        Character& m = party.members[i];
        Currency sum = 0;
        if (!addCurrencyChecked(dest.wallet.goldInBank, m.wallet.goldOnHand, sum)) {
            return false;
        }
        dest.wallet.goldInBank = sum;
        m.wallet.goldOnHand = 0;
    }
    return true;
}

bool partyDepositGold(Ctx& ctx, Party& party) {
    // PartyDeposit: m.GoldInBank += m.GoldOnHand; m.GoldOnHand = 0 (each member)
    // UNKNOWN: overflow handling for this step is not stated; stop on overflow like the pools.
    static_cast<void>(ctx);
    for (i32 i = 0; i < party.size; ++i) {
        Character& m = party.members[i];
        Currency sum = 0;
        if (!addCurrencyChecked(m.wallet.goldInBank, m.wallet.goldOnHand, sum)) {
            return false;
        }
        m.wallet.goldInBank = sum;
        m.wallet.goldOnHand = 0;
    }
    return true;
}

bool poolAndDepositGold(Ctx& ctx, Party& party) {
    // PoolAndDeposit:
    //   for m != to: to.GoldInBank += m.GoldInBank; to.GoldInBank += m.GoldOnHand;
    //                m.GoldInBank = m.GoldOnHand = 0
    //   to.GoldInBank += to.GoldOnHand; to.GoldOnHand = 0
    // Port deviation (c16): on overflow every balance is rolled back to its
    // starting value. The original has no error handler.
    static_cast<void>(ctx);
    const i32 to = party.selectedIndex;
    if (to < 0 || to >= party.size) return true;

    Arr<Purse, kPartyMaxSize> snapshot{};
    for (i32 i = 0; i < party.size; ++i) {
        snapshot[i] = party.members[i].wallet;
    }

    Character& dest = party.members[to];
    bool ok = true;
    for (i32 i = 0; i < party.size && ok; ++i) {
        if (i == to) continue;
        Character& m = party.members[i];
        Currency afterBank = 0;
        if (!addCurrencyChecked(dest.wallet.goldInBank, m.wallet.goldInBank, afterBank)) {
            ok = false;
            break;
        }
        Currency afterOnHand = 0;
        if (!addCurrencyChecked(afterBank, m.wallet.goldOnHand, afterOnHand)) {
            ok = false;
            break;
        }
        dest.wallet.goldInBank = afterOnHand;
        m.wallet.goldInBank = 0;
        m.wallet.goldOnHand = 0;
    }
    if (ok) {
        Currency finalBank = 0;
        if (!addCurrencyChecked(dest.wallet.goldInBank, dest.wallet.goldOnHand, finalBank)) {
            ok = false;
        } else {
            dest.wallet.goldInBank = finalBank;
            dest.wallet.goldOnHand = 0;
        }
    }

    if (!ok) {
        for (i32 i = 0; i < party.size; ++i) {
            party.members[i].wallet = snapshot[i];
        }
        return false;
    }
    return true;
}

PartyPoolOutcome poolPartyGold(Ctx& ctx, Party& party, PartyPoolKind kind, bool inCombat) {
    // Order confirmed by reference (Party.cs fn1521 case 80): combat is refused first,
    // then the lone member: "If fn0EEF() > 0 And Foes(0).Engaged Then ... Else If UBound(Chars) > m00A6 ...".
    if (inCombat) {
        ctx.say("party.pool.inCombat", "Cannot pool gold while in combat!", {}, {});
        return PartyPoolOutcome::InCombat;
    }
    // if Party.Size == 1: refuse(NobodyToPoolTo)
    if (party.size <= 1) {
        ctx.say("party.pool.alone", "Nobody else in party to Pool to!", {}, {});
        return PartyPoolOutcome::NobodyToPoolTo;
    }
    bool ok = true;
    switch (kind) {
        case PartyPoolKind::ToSelected:
            ok = poolGoldToSelected(ctx, party);
            break;
        case PartyPoolKind::ToBank:
            ok = poolGoldToBank(ctx, party);
            break;
        case PartyPoolKind::PartyDeposit:
            ok = partyDepositGold(ctx, party);
            break;
        case PartyPoolKind::PoolAndDeposit:
            ok = poolAndDepositGold(ctx, party);
            break;
    }
    // Completion line. Reference PoolCompleted(to) for ALT-p (ToSelected):
    // "Pool of party's on-hand gold to \"<name>\" completed!". Other kinds keep "Pool completed".
    // Printed whenever the party has two or more members, without checking whether gold moved (c16, c43).
    if (kind == PartyPoolKind::ToSelected) {
        ctx.say("party.pool.completed",
                "Pool of party's on-hand gold to \"<name>\" completed!", {},
                {party.members[party.selectedIndex].name});
    } else {
        ctx.say("party.pool.completed", "Pool completed", {}, {});
    }
    checkWealthiest(ctx, party);
    return ok ? PartyPoolOutcome::Completed : PartyPoolOutcome::OverflowStopped;
}

void checkWealthiest(Ctx& ctx, Party& party) {
    // UNKNOWN: CheckWealthiest body is not in the source chunks (c48).
    // Simplest sensible rule: the richest member (purse, ties to the earliest index)
    // becomes the gold leader.
    static_cast<void>(ctx);
    if (party.size <= 0) return;
    i32 best = 0;
    Currency bestPurse = party.members[0].purse();
    for (i32 i = 1; i < party.size; ++i) {
        const Currency p = party.members[i].purse();
        if (p > bestPurse) {
            bestPurse = p;
            best = i;
        }
    }
    party.goldLeaderIndex = best;
}

// ---------------------------------------------------------------------------
// Roster changes
// ---------------------------------------------------------------------------

bool removeMember(Ctx& ctx, Party& party, const Str& name) {
    // if character not in party or Members.Count == 1: false
    // remove; SelectedIndex clamped to [0, Count-1]
    static_cast<void>(ctx);
    if (party.size <= 1) return false;
    const i32 idx = findMemberIndex(party, name);
    if (idx < 0) return false;
    removeAt(party, idx);
    clampSelected(party);
    return true;
}

RearrangeOutcome rearrangeParty(Ctx& ctx, Party& party, i32 a, i32 b, bool inCombat) {
    // CONFLICT R16: the combat guard applies in the dungeon only (port comment, c43);
    // the city never refuses. The original's behaviour is not stated.
    if (inCombat && party.inDungeon) {
        ctx.say("party.rearrange.combat", "You cannot rearrange the party during combat!", {}, {});
        return RearrangeOutcome::InCombat;
    }
    if (a < 0 || a >= party.size || b < 0 || b >= party.size) {
        return RearrangeOutcome::OutOfRange;
    }
    if (a == b) {
        return RearrangeOutcome::SameSlot;
    }

    // swap(Members[a], Members[b]) (port fix: a true swap, not a rotation, c16)
    const Character tmp = party.members[a];
    party.members[a] = party.members[b];
    party.members[b] = tmp;

    // if SelectedIndex == a: SelectedIndex = b; elif == b: SelectedIndex = a
    if (party.selectedIndex == a) {
        party.selectedIndex = b;
    } else if (party.selectedIndex == b) {
        party.selectedIndex = a;
    }

    // announce NowLeader if a == 0 or b == 0
    if (a == 0 || b == 0) {
        ctx.say("party.leader.changed", "\"<name>\" is now party leader!", {},
                {party.members[0].name});
    }
    return RearrangeOutcome::Swapped;
}

bool selectMember(Ctx& ctx, Party& party, i32 slot) {
    // if slot == SelectedIndex or !Select(slot): return
    if (slot == party.selectedIndex) return false;
    if (!selectSlot(party, slot)) return false;

    // if leader.DungeonLevel <= 0 and !leader.IsDead: refresh the town screens
    const Character& leader = party.leader();
    if (leader.location.dungeonLevel <= 0 && !leader.isDead()) {
        // UI refresh of Store, Confinement, Morgue, Guild, Bank (c16).
        ctx.say("party.town.refresh", "Town screens refreshed", {}, {});
    }
    return true;
}

bool selectNextAble(Ctx& ctx, Party& party) {
    // Able = not IsDead and not Stoned and CurHits > 0
    // for step 1..Count: i = (SelectedIndex + step) % Count; if able(i): select i
    static_cast<void>(ctx);
    if (party.size <= 0) return false;
    for (i32 step = 1; step <= party.size; ++step) {
        const i32 i = (party.selectedIndex + step) % party.size;
        const Character& c = party.members[i];
        const bool able = !c.isDead() && c.deathState != DeathState::Stoned &&
                          c.vitals.curHits > 0;
        if (able) {
            party.selectedIndex = i;
            return true;
        }
    }
    return false;
}

void cycleSelectedMember(Ctx& ctx, Party& party) {
    // if Size > 1: Select((SelectedIndex + 1) % Size)  (silent, no UI refresh; c42, c48)
    static_cast<void>(ctx);
    if (party.size > 1) {
        selectSlot(party, (party.selectedIndex + 1) % party.size);
    }
}

// ---------------------------------------------------------------------------
// Combat orders and settings (c54)
// ---------------------------------------------------------------------------

void ensureDefaultCombatOrders(Ctx& ctx, Party& party) {
    // for c in Members: if order.Kind == None: order.ApplyDefault(c); keep order.Target
    static_cast<void>(ctx);
    for (i32 i = 0; i < party.size; ++i) {
        applyDefaultOrder(party.members[i]);
    }
}

CombatOrderKind combatSettingKind(i32 setting) {
    // 0 = Nothing, 1 = Fight, 2 = Defend, 3 + k = Buffer k
    if (setting <= kCombatSettingNothing) return CombatOrderKind::None;
    if (setting == kCombatSettingFight) return CombatOrderKind::Fight;
    if (setting == kCombatSettingDefend) return CombatOrderKind::Defend;
    return CombatOrderKind::Buffer;
}

i32 combatSettingBufferIndex(i32 setting) {
    // 3 + k -> k (0-based index into SpellBuffers); otherwise -1
    if (setting < kCombatSettingBufferBase) return -1;
    return setting - kCombatSettingBufferBase;
}

i32 combatSettingFromKind(CombatOrderKind kind, i32 bufferIndex) {
    switch (kind) {
        case CombatOrderKind::None:
            return kCombatSettingNothing;
        case CombatOrderKind::Fight:
            return kCombatSettingFight;
        case CombatOrderKind::Defend:
            return kCombatSettingDefend;
        case CombatOrderKind::Buffer:
            return kCombatSettingBufferBase + bufferIndex;
    }
    return kCombatSettingNothing;
}

Str partyStatusCode(const Character& c) {
    // s = ("Po" if poisoned) + ("Di" if diseased) + ("Pa" if paralysed)
    // return s == "" ? "OK" : s
    // UNKNOWN: "poisoned" and "diseased" are taken as level > 0.
    Str s;
    if (c.ailments.poisonLevel > 0) s += kStatusPoisoned;
    if (c.ailments.diseaseLevel > 0) s += kStatusDiseased;
    if (c.ailments.paralysed) s += kStatusParalysed;
    if (s.empty()) return Str(kStatusOk);
    return s;
}

// ---------------------------------------------------------------------------
// Autoload (c16)
// ---------------------------------------------------------------------------

AutoloadOutcome autoloadMember(Ctx& ctx, Party& party, Character& candidate, bool stillHere) {
    // UNKNOWN: check order here is not stated in the SRD; the order used is
    // not here, dead, full, leader dead, opposed, then join.
    if (!stillHere) {
        ctx.say("party.autoload.notHere",
                "AUTOLOAD PARTY: \"<name>\" is not here anymore!", {}, {candidate.name});
        return AutoloadOutcome::NotHere;
    }
    if (candidate.isDead()) {
        ctx.say("party.autoload.dead",
                "AUTOLOAD PARTY: \"<name>\" is dead!  Can't join!", {}, {candidate.name});
        return AutoloadOutcome::Dead;
    }
    if (party.size >= kPartyMaxSize) {
        ctx.say("party.autoload.full", "AUTOLOAD PARTY: Your party is too full!", {}, {});
        return AutoloadOutcome::Full;
    }
    if (party.size >= 1 && party.members[0].isDead()) {
        return AutoloadOutcome::LeaderDead;  // UNKNOWN: message not in the chunks
    }
    for (i32 i = 0; i < party.size; ++i) {
        if (autoloadAlignmentClash(party.members[i].alignment, candidate.alignment)) {
            ctx.say("party.autoload.opposed",
                    "AUTOLOAD PARTY: \"<name>\" is oppositely aligned to one or more members of the party!",
                    {}, {candidate.name});
            return AutoloadOutcome::Opposed;
        }
    }
    appendMember(party, candidate);
    ctx.say("party.autoload.joined",
            "AUTOLOAD PARTY: \"<name>\" is now a member of your party.", {}, {candidate.name});
    return AutoloadOutcome::Joined;
}

bool autoloadAlignmentClash(Align a, Align b) {
    // |member.Alignment - them.Alignment| > 1 (integer difference), c16.
    // CONFLICT: this matches opposedAlignment only when the enum is ordered
    // Good=0, Neutral=1, Evil=2, which is the order used by Align here.
    const i32 diff = static_cast<i32>(a) - static_cast<i32>(b);
    const i32 gap = diff < 0 ? -diff : diff;
    return gap > kAutoloadAlignGap;
}

// ---------------------------------------------------------------------------
// City join (c16, c48)
// ---------------------------------------------------------------------------

CityJoinOutcome joinFromCity(Ctx& ctx, Party& party, Character& who, const Str& said) {
    // if Members[0].IsDead: return
    if (party.size >= 1 && party.members[0].isDead()) {
        return CityJoinOutcome::LeaderDead;
    }
    // if Size >= Party.MaxSize: "Your party is too full!"
    if (party.size >= kPartyMaxSize) {
        ctx.say("party.join.full", "Your party is too full!", {}, {});
        return CityJoinOutcome::Full;
    }
    // UNKNOWN: placement of the AlreadyInParty and NotHere checks; they are
    // implied by the candidate list (SamePlace with the Leader, not in party).
    if (findMemberIndex(party, who.name) >= 0) {
        return CityJoinOutcome::AlreadyInParty;
    }
    // UNKNOWN: with an empty party there is no leader to compare with.
    if (party.size > 0 && !samePlace(party.leader(), who)) {
        ctx.say("party.join.notHere", "Character is not here!", {}, {});
        return CityJoinOutcome::NotHere;
    }
    // if HasPassword and not PasswordMatches(said): "Incorrect Password!"
    if (!passwordMatches(who, said)) {
        ctx.say("party.join.password", "Incorrect Password!", {}, {});
        return CityJoinOutcome::WrongPassword;
    }
    // if any member Opposed(member.Alignment, who.Alignment): refuse
    for (i32 i = 0; i < party.size; ++i) {
        if (opposedAlignment(party.members[i].alignment, who.alignment)) {
            ctx.say("party.join.opposed",
                    "\"<name>\" is oppositely aligned to one or more members of the party!",
                    {}, {who.name});
            return CityJoinOutcome::Opposed;
        }
    }
    // if who.IsDead and not PendingRaise.IsPending(who): "X is Dead! ..."
    // CONFLICT: the port admits dead joiners with a pending raise (CityWindow, c48);
    // the engine (Party.cs) refuses all dead joiners. The port rule is used here.
    if (who.isDead() && !who.pendingRaise) {
        ctx.say("party.join.deadNotPending", "\"<name>\" is Dead! ...", {}, {who.name});
        return CityJoinOutcome::DeadNotPending;
    }
    // who.CurHits = MaxHits; who.CurSpellPoints = MaxSpellPoints
    who.vitals.curHits = who.vitals.maxHits;
    who.vitals.curSpellPoints = who.vitals.maxSpellPoints;

    // UNKNOWN: the SRD omits the add itself; the join is placed before PruneTheDead.
    appendMember(party, who);
    // PruneTheDead(): a dead joiner with a pending raise is pruned here when more
    // than one member is listed. This follows the SRD order.
    pruneTheDead(ctx, party);
    return CityJoinOutcome::Joined;
}

bool passwordMatches(const Character& who, const Str& said) {
    // No password set: always matches. Otherwise compared as typed.
    // Passwords are stored in plain text (original quirk, c31).
    // UNKNOWN: case sensitivity is not stated; exact match is used.
    if (who.password.empty()) return true;
    return who.password == said;
}

bool isBodyInCity(const Character& body, const Character& self, const Party& party) {
    // BodiesInTheCity = dead && Name != self && DungeonLevel == 0 && no party member carries them
    if (!body.isDead()) return false;
    if (body.name == self.name) return false;
    if (body.location.dungeonLevel != 0) return false;
    for (i32 i = 0; i < party.size; ++i) {
        if (party.members[i].carryingCharacterName == body.name) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Give gold and items (c10, c29)
// ---------------------------------------------------------------------------

PartyGiveGoldOutcome giveGoldToMember(Ctx& ctx,
                                      Party& party,
                                      Character& giver,
                                      i32 receiverIndex,
                                      Currency amount) {
    // Nobody in party to give to (c10)
    if (party.size <= 1 || receiverIndex < 0 || receiverIndex >= party.size) {
        ctx.say("party.give.nobody", "Nobody in party to give to!", {}, {});
        return PartyGiveGoldOutcome::NobodyToGiveTo;
    }
    // Gold "" or Val < 1 -> 0 (c10). UNKNOWN: the 1-gold floor is taken as
    // currencyFromWhole(1) in Currency units.
    if (amount < currencyFromWhole(1)) {
        return PartyGiveGoldOutcome::NothingToGive;
    }
    // Val > gold on hand -> "Not enough gold!" (c10)
    if (amount > giver.wallet.goldOnHand) {
        ctx.say("party.give.notEnough", "Not enough gold!", {}, {});
        return PartyGiveGoldOutcome::NotEnoughGold;
    }
    Character& receiver = party.members[receiverIndex];
    giver.wallet.goldOnHand -= amount;
    receiver.wallet.goldOnHand += amount;
    // Give of N gold to "X" Completed! (nums[0] = N, strs[0] = receiver name)
    ctx.say("party.give.goldDone", "Give of gold to \"<name>\" Completed!",
            {static_cast<i64>(currencyWhole(amount))}, {receiver.name});
    return PartyGiveGoldOutcome::Given;
}

i32 itemSlotIndex(i32 n) {
    // slot = (n > 1) ? n - 1 : 0
    return (n > 1) ? n - 1 : 0;
}

PartyGiveItemOutcome giveItemToMember(Ctx& ctx,
                                      Party& party,
                                      Character& giver,
                                      i32 receiverIndex,
                                      i32 packSlot) {
    // Nobody in party to give to (c10)
    if (party.size <= 1 || receiverIndex < 0 || receiverIndex >= party.size) {
        ctx.say("party.give.nobody", "Nobody in party to give to!", {}, {});
        return PartyGiveItemOutcome::NobodyToGiveTo;
    }
    const i32 slot = itemSlotIndex(packSlot);
    // Item "" or "0" and slot 1 (Hands) -> "No Item to give!" (c10)
    if (slot == kGiveHandsSlot || slot < 0 || slot >= kPackSlots) {
        ctx.say("party.give.noItem", "No Item to give!", {}, {});
        return PartyGiveItemOutcome::NoItemToGive;
    }
    // Locate the giver's member index by address identity (no RTTI).
    i32 giverIndex = -1;
    for (i32 i = 0; i < party.size; ++i) {
        if (&party.members[i] == &giver) {
            giverIndex = i;
            break;
        }
    }
    if (giverIndex < 0) {
        ctx.say("party.give.nobody", "Nobody in party to give to!", {}, {});
        return PartyGiveItemOutcome::NobodyToGiveTo;
    }
    Inventory& from = party.inventories[giverIndex];
    Inventory& to = party.inventories[receiverIndex];
    // Item "" or "0" -> "No Item to give!" (c10)
    if (from.pack[slot].id == 0) {
        ctx.say("party.give.noItem", "No Item to give!", {}, {});
        return PartyGiveItemOutcome::NoItemToGive;
    }
    // UNKNOWN: the Equipped check and the receiver placement rule (first empty
    // slot, Hands excluded or not) are not given in the SRD text provided.
    // Here the item goes to the receiver's first empty pack slot.
    i32 dest = -1;
    for (i32 s = 0; s < kPackSlots; ++s) {
        if (to.pack[s].id == 0) {
            dest = s;
            break;
        }
    }
    if (dest < 0) {
        // UNKNOWN: no SRD message for a full pack. Reuses the no-item outcome.
        ctx.say("party.give.packFull", "Pack is full!", {}, {});
        return PartyGiveItemOutcome::NoItemToGive;
    }
    to.pack[dest] = from.pack[slot];
    from.pack[slot] = Item{};
    // RESOLVED-BY-REFERENCE: the completion line reads "Give of Item to "X" Completed!"
    // (GiveCommand.cs:172, sub14B5 3004-3006), with a capital I. Strs[0] = receiver name.
    ctx.say("party.give.itemDone", "Give of Item to \"<name>\" Completed!", {},
            {party.members[receiverIndex].name});
    return PartyGiveItemOutcome::Given;
}

// ---------------------------------------------------------------------------
// Leaving and joining details (c16, c43)
// ---------------------------------------------------------------------------

PartyLeaveOutcome leaveParty(Ctx& ctx, Party& party, bool confirmed, bool monstersInArea) {
    // if SelectedIndex == 0: "Leader cannot leave Party!"
    if (party.selectedIndex == 0) {
        ctx.say("party.leave.leader", "Leader cannot leave Party!", {}, {});
        return PartyLeaveOutcome::LeaderCannotLeave;
    }
    // UNKNOWN: order of the monsters check and the confirm check.
    if (monstersInArea) {
        ctx.say("party.leave.monsters",
                "\"<name>\" cannot leave while monsters are in the area!", {},
                {party.members[party.selectedIndex].name});
        return PartyLeaveOutcome::MonstersInArea;
    }
    if (!confirmed) {
        return PartyLeaveOutcome::NotConfirmed;
    }
    // if confirmed: party.Remove(who); PruneTheDead()
    removeAt(party, party.selectedIndex);
    clampSelected(party);
    pruneTheDead(ctx, party);
    return PartyLeaveOutcome::Left;
}

void setupJoinerInCombat(Ctx& ctx, Party& party, Character& newcomer) {
    // UNKNOWN: sub0BDE body is not in the source chunks (c59).
    // Minimal setup for a living joiner: default combat order if none is set.
    // Dead joiners are refused before this is reached (c59).
    static_cast<void>(ctx);
    static_cast<void>(party);
    if (newcomer.isDead()) return;
    applyDefaultOrder(newcomer);
}

DefaultOptionsOutcome setDefaultCombatOptions(Ctx& ctx,
                                              Party& party,
                                              bool inCombat,
                                              bool monstersInArea) {
    // Refusals in combat and with monsters present (c43).
    if (inCombat) {
        ctx.say("party.options.inCombat",
                "Cannot set default options while in Combat!", {}, {});
        return DefaultOptionsOutcome::InCombat;
    }
    if (monstersInArea) {
        // UNKNOWN: exact wording; the source gives "...monsters are in the area!".
        ctx.say("party.options.monsters",
                "Cannot set default options while monsters are in the area!", {}, {});
        return DefaultOptionsOutcome::MonstersInArea;
    }
    // UNKNOWN: the apply step is not in the chunks; fill empty orders with the default.
    ensureDefaultCombatOrders(ctx, party);
    return DefaultOptionsOutcome::Applied;
}

PartySaveAction disbandParty(Ctx& ctx, Party& party, bool rememberForReload) {
    // Save only at disband, with remember = (answer == Yes). Not remembered: the
    // saved party file is deleted (c16).
    // UNKNOWN: the caller clears the party after writing or deleting the file, so the
    // members are not cleared here. Disband behaviour varies by version (c91).
    static_cast<void>(party);
    ctx.say("party.disband.start", "Disbanding Party...", {}, {});
    return rememberForReload ? PartySaveAction::SaveParty : PartySaveAction::DeleteSavedParty;
}

i32 townGoldLeaderIndex(const Party& party) {
    // First living member in the city (dungeon level 0), in party order (c36).
    // Returns -1 when nobody qualifies. UNKNOWN: the -1 result is not in the SRD.
    for (i32 i = 0; i < party.size; ++i) {
        const Character& m = party.members[i];
        if (!m.isDead() && m.location.dungeonLevel == 0) return i;
    }
    return -1;
}

bool commandSwitchSlotValid(i32 slot, i32 partySize, bool inDungeon) {
    // CONFLICT: Dungeon uses 1-based Alt+1..4 keys (slot exists when slot <= partySize,
    // size > 1). City accepts slot 0 or higher (c43, c48). The index base in the two
    // places differs and is not confirmed. UNKNOWN: whether Alt+0 in the city does nothing.
    if (inDungeon) {
        return partySize > 1 && slot >= 1 && slot <= partySize &&
               slot <= kCommandSwitchSlotsMax;
    }
    return slot >= 0;
}

}  // namespace mordor
