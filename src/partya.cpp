// partya.cpp: party membership, join and leave, pruning of the dead, group
// turn targeting, disband and autoload, and group commands.
// Implements include/mordor/partya.h from SRD chapter 17 (part c17p1), rules R1-R30.
#include "mordor/partya.h"

#include <string>

#include "mordor/items1b.h"  // FIXED: SwapResult is declared in items1b.h
#include "mordor/items2b.h"
#include "mordor/math_util.h"

namespace mordor {

namespace {

// R20/R23/R24: balances are Currency in 1/10000 gold.
// UNKNOWN: the bank overflow limit is not stated in this SRD part; 2,000,000,000 gold is used.
constexpr Currency kBalanceCap = 2000000000LL * 10000LL;

Character& memberAt(Party& p, i32 i) { return p.members[static_cast<u32>(i)]; }
const Character& memberAt(const Party& p, i32 i) { return p.members[static_cast<u32>(i)]; }

Str numText(i64 v) { return std::to_string(static_cast<long long>(v)); }

// R21: membership is by name, since characters carry no separate id.
bool isMemberOf(const Party& party, const Character& c) {
    for (i32 i = 0; i < party.size; ++i) {
        if (memberAt(party, i).name == c.name) return true;
    }
    return false;
}

// Removes the member at index; later members and their standing orders shift down.
void eraseMember(Party& party, i32 index) {
    if (party.size <= 0) return;
    for (i32 j = index; j + 1 < party.size; ++j) {
        memberAt(party, j) = memberAt(party, j + 1);
        party.combatOrders[static_cast<u32>(j)] = party.combatOrders[static_cast<u32>(j + 1)];
        party.itemOrders[static_cast<u32>(j)] = party.itemOrders[static_cast<u32>(j + 1)];
    }
    const u32 last = static_cast<u32>(party.size - 1);
    party.combatOrders[last] = CombatOrder{};
    party.itemOrders[last] = ItemOrder{};
    party.size -= 1;
}

// Appends a member at the end of the party (no-op when full).
void appendMember(Party& party, const Character& c) {
    if (party.size >= kPartyMaxSize) return;
    memberAt(party, party.size) = c;
    party.combatOrders[static_cast<u32>(party.size)] = CombatOrder{};
    party.itemOrders[static_cast<u32>(party.size)] = ItemOrder{};
    party.size += 1;
}

// R11, R12: prunes one member. Returns true when the member was removed.
// announce=false is the quiet form used by town autoload (R13).
bool pruneMemberAt(Ctx& ctx, Party& party, i32 index, bool announce) {
    if (party.size <= 1) return false;                                   // R11: lone member kept
    if (index < 0 || index >= party.size) return false;
    {
        const Character& m = memberAt(party, index);
        const bool alive = !m.isDead();
        if (alive && m.vitals.curHits > 0 && m.deathState != DeathState::Stoned) {
            return false;                                                // R12.1: stays
        }
    }
    Character& m = memberAt(party, index);
    m.vitals.curHits = 0;                                                // R12.2
    // UNKNOWN: "level" in R12.2 is read as the dungeon level the member stands on.
    if (announce && m.location.dungeonLevel == memberAt(party, 0).location.dungeonLevel) {
        ctx.say("party.member.died", "\"" + m.name + "\" has died!", {}, {});  // R12.2
    }
    if (!m.isDead()) m.deathState = DeathState::Dead;                    // R12.2
    m.saved = true;                                                      // R12.2: saved
    eraseMember(party, index);                                           // R12.2: removed
    if (party.size > 0) {
        // R12.2: remaining members move to the leader's level, X and Y.
        const Location lead = memberAt(party, 0).location;
        for (i32 j = 0; j < party.size; ++j) {
            Location& l = memberAt(party, j).location;
            l.dungeonLevel = lead.dungeonLevel;
            l.x = lead.x;
            l.y = lead.y;
        }
        party.dungeonLevel = lead.dungeonLevel;
        party.x = lead.x;
        party.y = lead.y;
    }
    return true;
}

// R12.3: the walk does not advance after a removal.
void pruneTheDeadLoop(Ctx& ctx, Party& party, bool announce) {
    i32 i = 0;
    while (i < party.size) {
        if (!pruneMemberAt(ctx, party, i, announce)) ++i;
    }
}

// sub0930 companion drop (R15.2, R17). UNKNOWN: the companion drop semantics are not
// stated in this SRD part; owned companions are released from their owner.
void dropOwnedCompanions(Vec<Companion>& companions, i32 partySize) {
    const i32 n = static_cast<i32>(companions.size());
    for (i32 i = 0; i < n; ++i) {
        Companion& c = companions[static_cast<u32>(i)];
        if (c.occupied && c.owner >= 0 && c.owner < partySize) {
            c.occupied = false;
            c.owner = -1;
        }
    }
}

// R22.3 / R24: all on-hand gold is pooled and shared evenly; the remainder goes to one member.
// UNKNOWN: the pool formula is in the SRD Formulas section, not in this part.
void distributeOnHand(Party& party, i32 remainderIndex) {
    const i32 n = party.size;
    if (n <= 0) return;
    Currency total = 0;
    for (i32 i = 0; i < n; ++i) total += memberAt(party, i).wallet.goldOnHand;
    const Currency share = total / n;
    const Currency rem = total % n;
    for (i32 i = 0; i < n; ++i) memberAt(party, i).wallet.goldOnHand = share;
    const i32 r = (remainderIndex >= 0 && remainderIndex < n) ? remainderIndex : 0;
    memberAt(party, r).wallet.goldOnHand += rem;
}

// R24 Party Deposit: every member's on-hand gold goes to the bank.
void depositAll(Party& party) {
    for (i32 i = 0; i < party.size; ++i) {
        Purse& w = memberAt(party, i).wallet;
        w.goldInBank += w.goldOnHand;
        w.goldOnHand = 0;
    }
}

// R23.4: the transfer removes gold from on hand; any on-hand deficit comes out of the bank.
void drawExchangeGold(Character& who, Currency amount) {
    who.wallet.goldOnHand -= amount;
    if (who.wallet.goldOnHand < 0) {
        who.wallet.goldInBank += who.wallet.goldOnHand;
        who.wallet.goldOnHand = 0;
    }
}

// R25.1: parses one signed integer from text[from, to), allowing surrounding spaces.
bool parseSignedInt(const Str& text, i32 from, i32 to, i32& out) {
    i32 i = from;
    i32 end = to;
    while (i < end && text[static_cast<u32>(i)] == ' ') ++i;
    while (end > i && text[static_cast<u32>(end - 1)] == ' ') --end;
    bool neg = false;
    if (i < end && text[static_cast<u32>(i)] == '-') {
        neg = true;
        ++i;
    }
    if (i >= end) return false;
    i64 v = 0;
    for (; i < end; ++i) {
        const char ch = text[static_cast<u32>(i)];
        if (ch < '0' || ch > '9') return false;
        v = v * 10 + (ch - '0');
    }
    out = static_cast<i32>(neg ? -v : v);
    return true;
}

}  // namespace

// R2: Good and Evil cannot share a party.
bool oppositelyAligned(Align a, Align b) {
    return (a == Align::Good && b == Align::Evil) || (a == Align::Evil && b == Align::Good);
}

// R4.3: the party is full at kPartyMaxSize.
bool isPartyFull(const Party& party) {
    return party.size >= kPartyMaxSize;
}

// R4, R6, R7, R8: join preconditions, in SRD order. Messages with SRD text are emitted here.
JoinCheck checkJoin(Ctx& ctx, const Party& party, const Character& joiner,
                    bool inDungeon, bool inCombat, bool passwordMatches) {
    JoinCheck out;
    const bool joinerDead = joiner.isDead();
    if (party.size > 0 && memberAt(party, 0).isDead()) {                 // R4.1 silent
        out.refusal = JoinRefusal::LeaderDead;
        return out;
    }
    if (inDungeon && inCombat && joinerDead) {                           // R4.2
        out.refusal = JoinRefusal::DeadInCombat;
        ctx.say("party.join.combat", "Cannot raise character in Combat!", {}, {});
        return out;
    }
    if (isPartyFull(party)) {                                            // R4.3
        out.refusal = JoinRefusal::PartyFull;
        ctx.say("party.join.full", "Your party is too full!", {}, {});
        return out;
    }
    if (!joiner.password.empty() && !passwordMatches) {                  // R6
        out.refusal = JoinRefusal::WrongPassword;
        ctx.say("party.join.password", "Incorrect Password!", {}, {});
        return out;
    }
    if (hasOppositeMember(party, joiner.alignment)) {                    // R7 (no message in SRD)
        out.refusal = JoinRefusal::OppositeAlignment;
        return out;
    }
    if (joinerDead && !joiner.pendingRaise) {                            // R8
        out.refusal = JoinRefusal::DeadNoPendingRaise;
        // UNKNOWN: the SRD text ends in "..."; the tail is not given.
        ctx.say("party.join.dead", "\"" + joiner.name + "\" is Dead! ...", {}, {});
        return out;
    }
    out.runSub0BDE = inDungeon && inCombat && !joinerDead;               // R4.2 living joiner
    return out;
}

// R5: a candidate is at the leader's place (or anywhere in town) and not already in the party.
bool isJoinCandidate(const Party& party, const Character& c, bool inCity) {
    return !isMemberOf(party, c) && samePlace(party, c, inCity);
}

// R21.4: same square as the leader. In town everyone counts as here.
bool samePlace(const Party& party, const Character& c, bool bothInCity) {
    if (bothInCity) return true;
    if (party.size <= 0) return false;
    const Location& l = memberAt(party, 0).location;
    return c.location.x == l.x && c.location.y == l.y &&
           c.location.dungeonLevel == l.dungeonLevel && c.location.area == l.area;
}

// R5 (dungeon): on file, not in the party, on the current floor and area; dead included.
bool isOnJoinList(const Party& party, const Character& c, i32 currentArea) {
    if (isMemberOf(party, c)) return false;
    if (party.size <= 0) return false;
    return c.location.dungeonLevel == party.dungeonLevel && c.location.area == currentArea;
}

// R7: any member oppositely aligned to the joiner.
bool hasOppositeMember(const Party& party, Align joinerAlignment) {
    for (i32 i = 0; i < party.size; ++i) {
        if (oppositelyAligned(memberAt(party, i).alignment, joinerAlignment)) return true;
    }
    return false;
}

// R9: the newcomer takes the leader's square, copies the lost flags, gets full HP and SP,
// joins, runs the companion carry, then the dead are pruned (R13, R14).
void performJoin(Ctx& ctx, Party& party, Vec<Companion>& companions, Character& joiner) {
    if (party.size <= 0 || isPartyFull(party)) return;
    const Character& leader = memberAt(party, 0);
    joiner.location.x = leader.location.x;                               // R9 square
    joiner.location.y = leader.location.y;
    joiner.location.dungeonLevel = leader.location.dungeonLevel;
    joiner.location.area = leader.location.area;
    joiner.lost.level = leader.lost.level;                               // R9 lost flags
    joiner.lost.position = leader.lost.position;
    joiner.lost.facing = leader.lost.facing;
    joiner.vitals.curHits = joiner.vitals.maxHits;                       // R9 full HP
    joiner.vitals.curSpellPoints = joiner.vitals.maxSpellPoints;         // R9 full SP
    // UNKNOWN: a dead joiner with a pending raise is pruned here (R12.1 treats isDead as
    // not alive); the raise is assumed to have been applied before the join.
    appendMember(party, joiner);
    companionCarryRun(ctx, party, companions);                           // R13: prune after carry
    pruneTheDeadLoop(ctx, party, true);                                  // R9, R14
}

// R10: leave. Refusals (monsters, leader) come before the confirmation (R10.3).
LeaveResult leaveParty(Ctx& ctx, Party& party, i32 slot,
                       bool inDungeon, bool monstersPresent, bool confirmed) {
    LeaveResult r;
    if (slot < 0 || slot >= party.size) {                                // UNKNOWN: bad slot, no effect
        r.outcome = LeaveOutcome::NotConfirmed;
        return r;
    }
    const Character& who = memberAt(party, slot);
    if (inDungeon && monstersPresent) {                                  // R10.1
        r.outcome = LeaveOutcome::RefusedMonstersPresent;
        ctx.say("party.leave.monsters",
                "\"" + who.name + "\" cannot leave while monsters are in the area!", {}, {});
        return r;
    }
    if (slot == kLeaderIndex) {                                          // R10.2
        r.outcome = LeaveOutcome::RefusedLeader;
        ctx.say("party.leave.leader", "Leader cannot leave Party!", {}, {});
        return r;
    }
    if (!confirmed) {                                                    // R10.3
        r.outcome = LeaveOutcome::NotConfirmed;
        return r;
    }
    Character leaver = who;
    leaver.saved = true;                                                 // R10.3 saved at current level and square
    leaver.location.x = party.x;                                         // R10.3 current square
    leaver.location.y = party.y;                                         // R10.3
    leaver.location.dungeonLevel = party.dungeonLevel;                   // R10.3 current level
    eraseMember(party, slot);
    // UNKNOWN: selected index is reset to the first member when it falls off the end.
    if (party.selectedIndex >= party.size) party.selectedIndex = 0;
    pruneTheDeadLoop(ctx, party, true);                                  // R10.3, R13
    r.outcome = LeaveOutcome::Left;
    r.leaver = leaver;
    return r;
}

// R11, R12: prunes one member at index. Returns true when the member was removed.
bool pruneOneMember(Ctx& ctx, Party& party, i32 index) {
    return pruneMemberAt(ctx, party, index, true);
}

// R13: prune after teleporter or chute only if the party size dropped.
bool pruneOnSizeDrop(i32 sizeBefore, i32 sizeAfter) {
    return sizeAfter < sizeBefore;
}

// R14: if any member is dead, one UPDATE fires and its callback removes the dead.
bool firePartyUpdateIfDead(Ctx& ctx, Party& party) {
    bool anyDead = false;
    for (i32 i = 0; i < party.size; ++i) {
        if (memberAt(party, i).isDead()) anyDead = true;
    }
    if (!anyDead) return false;
    pruneTheDeadLoop(ctx, party, true);
    return true;
}

// R15: wiped party. Size above 1 prunes (leaves one member); size 1 runs sub0930.
void handleWipedParty(Ctx& ctx, Party& party, Vec<Companion>& companions) {
    party.wiped = true;
    if (party.size > 1) {
        pruneTheDeadLoop(ctx, party, true);                              // R15.1
    } else if (party.size == 1) {
        dropOwnedCompanions(companions, party.size);                     // R15.2
    }
}

// R16: weighted roll over the members, using kTargetWeights by member index.
// UNKNOWN: the weighting formula is in the SRD Formulas section, not in this part.
i32 pickMonsterTarget(Ctx& ctx, const Party& party) {
    i32 n = party.size;
    if (n <= 0) return kLeaderIndex;
    if (n > kTargetWeightCount) n = kTargetWeightCount;
    i32 total = 0;
    for (i32 i = 0; i < n; ++i) total += kTargetWeights[i];
    if (total <= 0) return kLeaderIndex;
    const i32 roll = static_cast<i32>(ctx.rng.below(total));
    i32 acc = 0;
    for (i32 i = 0; i < n; ++i) {
        acc += kTargetWeights[i];
        if (roll < acc) return i;
    }
    return kLeaderIndex;
}

// R17: if the picked character reaches 0 hits, companions are dropped (sub0930) and the
// list is pruned. A lone character's death ends the monster turn; one member left stops the group.
GroupTurnStep afterTargetKnockedOut(Ctx& ctx, Party& party, Vec<Companion>& companions,
                                    i32 targetIndex) {
    if (targetIndex < 0 || targetIndex >= party.size) return GroupTurnStep::Continue;
    if (memberAt(party, targetIndex).vitals.curHits > 0) return GroupTurnStep::Continue;
    const i32 before = party.size;
    dropOwnedCompanions(companions, party.size);                         // R17 sub0930
    pruneTheDeadLoop(ctx, party, true);                                  // R17 prune
    if (before == 1) return GroupTurnStep::MonsterTurnEnds;              // R17 lone death
    if (party.size == 1) return GroupTurnStep::GroupStops;               // R17 one left
    return GroupTurnStep::Continue;
}

// R18: keys q and Exit are refused while monsters stand, unless the leader is dead.
// UNKNOWN: "the character" in R18 is read as the leader.
DisbandPlan planDisband(Ctx& ctx, const Party& party, bool monstersStanding) {
    (void)ctx;
    DisbandPlan plan;
    const bool leaderDead = party.size > 0 && memberAt(party, 0).isDead();
    plan.leaderDead = leaderDead;                                        // R18.5
    if (monstersStanding && !leaderDead) {
        plan.refused = true;
        return plan;
    }
    plan.askReload = party.size > 1;                                     // R18.1
    return plan;
}

// R18.2 to R18.5: what happens to the leader's record.
RecordAction resolveDisband(const Party& party, bool reloadWhenLoaded) {
    if (party.size > 0 && memberAt(party, 0).isDead()) return RecordAction::None;   // R18.5
    if (party.size <= 1) return RecordAction::BlankRecord;                           // R18.3
    return reloadWhenLoaded ? RecordAction::WriteRecord : RecordAction::DeleteRecord; // R18.2
}

// R19: autoload of remembered members. All lines carry the "AUTOLOAD PARTY: " prefix (R19.7).
AutoloadResult autoloadParty(Ctx& ctx, Party& party, Vec<Companion>& companions,
                             const Vec<RememberedMember>& remembered) {
    AutoloadResult result;
    // UNKNOWN: with no leader loaded, the party is treated as leader dead.
    if (party.size <= 0 || memberAt(party, 0).isDead()) {                // R19.1
        result.leaderDead = true;
        return result;
    }
    const i32 n = static_cast<i32>(remembered.size());
    for (i32 k = 0; k < n; ++k) {
        const RememberedMember& rm = remembered[static_cast<u32>(k)];
        const Character& c = rm.character;
        if (c.name == memberAt(party, 0).name) continue;                 // R19: other than the leader
        if (!rm.loadOk) {                                                // R19.2
            result.outcomes.push_back(AutoloadMemberOutcome::LoadFailed);
            ctx.say("party.autoload.error",
                    "AUTOLOAD PARTY: An error occurred auto-loading \"" + c.name + "\".", {}, {});
            continue;
        }
        const Location& leaderLoc = memberAt(party, 0).location;
        if (c.location.area != leaderLoc.area || c.location.dungeonLevel != leaderLoc.dungeonLevel) {
            result.outcomes.push_back(AutoloadMemberOutcome::NotHere);   // R19.3
            ctx.say("party.autoload.nothere",
                    "AUTOLOAD PARTY: \"" + c.name + "\" is not here anymore!", {}, {});
            continue;
        }
        if (c.isDead()) {                                                // R19.4
            result.outcomes.push_back(AutoloadMemberOutcome::Dead);
            ctx.say("party.autoload.dead",
                    "AUTOLOAD PARTY: \"" + c.name + "\" is dead!  Can't join!", {}, {});
            continue;
        }
        if (hasOppositeMember(party, c.alignment)) {                     // R19.5
            result.outcomes.push_back(AutoloadMemberOutcome::Opposite);
            ctx.say("party.autoload.opposite",
                    "AUTOLOAD PARTY: \"" + c.name +
                    "\" is oppositely aligned to one or more members of the party!", {}, {});
            continue;
        }
        if (isPartyFull(party)) break;                                   // UNKNOWN: full party not in R19; stop
        appendMember(party, c);                                          // R19.6 join
        ctx.say("party.autoload.joined",
                "AUTOLOAD PARTY: \"" + c.name + "\" is now a member of your party.", {}, {});
        companionCarryRun(ctx, party, companions);                       // R19.6
        result.outcomes.push_back(AutoloadMemberOutcome::Joined);
        result.joined += 1;
    }
    pruneTheDeadLoop(ctx, party, false);                                 // R13: town autoload prunes quietly
    return result;
}

// R20.1: a lone giver has nobody to give to.
bool giveAvailable(Ctx& ctx, const Party& party) {
    if (party.size <= 1) {
        ctx.say("party.give.alone", "Nobody in party to give to!", {}, {});
        return false;
    }
    return true;
}

// R20 gold answer: a value below 1 gold gives 0 silently; above on-hand gold is refused.
GiveGoldOutcome giveGoldTransfer(Ctx& ctx, Character& giver, Character& receiver, Currency amount) {
    if (currencyWhole(amount) < 1) return GiveGoldOutcome::ZeroNoOp;
    if (amount > giver.wallet.goldOnHand) {
        ctx.say("party.give.notenough", "Not enough gold!", {}, {});
        return GiveGoldOutcome::NotEnoughGold;
    }
    // R24: receiver balance cap. Refuse the whole give; giver keeps the gold.
    // RESOLVED-BY-REFERENCE: GiveCommand.cs Give() (sub14B5 has no handler): an overflow "goes to
    // fn1521's trap ... writes to the error file only and ends the command ... no line, no item".
    // So the receiver-full refusal is silent.
    // UNKNOWN: GiveGoldOutcome has no receiver-full value as provided; NotEnoughGold is a stand-in.
    if (amount > kBalanceCap - receiver.wallet.goldOnHand) {
        return GiveGoldOutcome::NotEnoughGold;
    }
    giver.wallet.goldOnHand -= amount;
    receiver.wallet.goldOnHand += amount;
    ctx.say("party.give.gold",
            "Give of " + numText(currencyWhole(amount)) + " gold to \"" + receiver.name + "\" Completed!",
            {}, {});
    return GiveGoldOutcome::Done;
}

// R22: pool gold (ALT-p).
PoolOutcome poolGold(Ctx& ctx, Party& party, i32 selectedIndex, bool inCombat) {
    if (party.size <= 1) {                                               // R22.1
        ctx.say("party.pool.alone", "Nobody else in party to Pool to!", {}, {});
        return PoolOutcome::RefusedAlone;
    }
    if (inCombat) {                                                      // R22.2
        ctx.say("party.pool.combat", "Cannot pool gold while in combat!", {}, {});
        return PoolOutcome::RefusedInCombat;
    }
    distributeOnHand(party, selectedIndex);                              // R22.3
    if (party.size >= 2) ctx.say("party.pool.done", "Pool completed", {}, {});
    checkWealthiest(ctx, party);                                         // R22.4
    // R22.5: a mod may replace this pool with Bank Party Pool & Deposit; mods are not modelled here.
    return PoolOutcome::Pooled;
}


// R23.2: dungeon uses on-hand gold; city adds the bank.
Currency exchangeAvailable(const Character& c, bool inDungeon) {
    return inDungeon ? c.wallet.goldOnHand : c.wallet.goldOnHand + c.wallet.goldInBank;
}

// R23: exchange gold transfer (FRMEXCHA). Negative amounts are not rejected (R23.5).
GoldTransferOutcome transferExchangeGold(Ctx& ctx, Character& from, Character& to,
                                         Currency amount, bool inDungeon) {
    if (amount == 0) return GoldTransferOutcome::ZeroNoOp;               // R23.1
    if (amount > exchangeAvailable(from, inDungeon)) {                   // R23.3
        ctx.say("party.exchange.toomuch", "Too Much!", {}, {});
        return GoldTransferOutcome::TooMuch;
    }
    drawExchangeGold(from, amount);                                      // R23.4
    to.wallet.goldOnHand += amount;
    return GoldTransferOutcome::Done;
}

// R24: the three bank party options are silent. On overflow every balance is rolled back.
BankPartyOutcome bankPartyOption(Ctx& ctx, Party& party, BankPartyOption option) {
    (void)ctx;
    const bool pool = option == BankPartyOption::PartyPool || option == BankPartyOption::PartyPoolAndDeposit;
    const bool deposit = option == BankPartyOption::PartyDeposit || option == BankPartyOption::PartyPoolAndDeposit;

    // R24: decide overflow on projected balances BEFORE touching any wallet, so a refused
    // operation leaves the party exactly as it was (no snapshot/rollback needed).
    // UNKNOWN: assumes distributeOnHand pools all on-hand gold into the selected member and
    // depositAll moves each member's on-hand gold into their bank.
    bool overflow = false;
    Currency pooled = 0;
    if (pool) {
        for (i32 i = 0; i < party.size && !overflow; ++i) {
            const Currency g = memberAt(party, i).wallet.goldOnHand;
            if (g > 0) {
                if (g > kBalanceCap - pooled) {
                    overflow = true;
                } else {
                    pooled += g;
                }
            }
        }
    }
    for (i32 i = 0; i < party.size && !overflow; ++i) {
        const Purse& w = memberAt(party, i).wallet;
        const Currency onHand = pool ? (i == party.selectedIndex ? pooled : Currency(0)) : w.goldOnHand;
        if (deposit && onHand > 0 && onHand > kBalanceCap - w.goldInBank) {
            overflow = true;
        }
    }
    if (overflow) {                                                      // R24 roll back (nothing changed yet)
        return BankPartyOutcome::RolledBack;
    }
    if (pool) {
        distributeOnHand(party, party.selectedIndex);
    }
    if (deposit) {
        depositAll(party);
    }
    return BankPartyOutcome::Done;
}

// R25.1: "a,b" with at most 5 characters and exactly one comma, each side one integer.
SwapInputResult parseSwapInput(const Str& text) {
    SwapInputResult r;
    if (text.size() > 5) return r;
    const i32 n = static_cast<i32>(text.size());
    i32 comma = -1;
    i32 commas = 0;
    for (i32 i = 0; i < n; ++i) {
        if (text[static_cast<u32>(i)] == ',') {
            comma = i;
            ++commas;
        }
    }
    if (commas != 1) return r;
    i32 a = 0;
    i32 b = 0;
    if (!parseSignedInt(text, 0, comma, a)) return r;
    if (!parseSignedInt(text, comma + 1, n, b)) return r;
    r.ok = true;
    r.first = a;
    r.second = b;
    return r;
}

// R25: swap items (fn1521).
bool runItemSwap(Ctx& ctx, Character& c, Inventory& inv, const Str& text, bool buffersFollow) {
    const SwapInputResult in = parseSwapInput(text);
    if (!in.ok) return false;                                            // R25.1 silent refusal
    // UNKNOWN: slot 1 (hands) is refused here; the SRD also says SwapSlots checks the range.
    if (in.first == 1 || in.second == 1) return false;                   // R25.2
    const SwapResult r = swapSlots(ctx, c, inv, in.first, in.second, buffersFollow);
    // UNKNOWN: SwapResult enumerator names are truncated in the API index; value 0 is assumed success.
    if (static_cast<i32>(r) != 0) return false;
    ctx.say("party.swap.done", "Item swap completed!", {}, {});          // R25.3
    return true;
}

// R26: swap party positions, with standing orders and the selected index following the character.
PartySwapOutcome swapPartyPositions(Ctx& ctx, Party& party, i32 a, i32 b,
                                    bool inDungeon, bool inCombat) {
    if (a < 0 || b < 0 || a >= party.size || b >= party.size) return PartySwapOutcome::InvalidIndex;
    if (inDungeon && inCombat) {                                         // R26.4
        ctx.say("party.swap.combat", "You cannot rearrange the party during combat!", {}, {});
        return PartySwapOutcome::RefusedInCombat;
    }
    if (a == b) return PartySwapOutcome::Swapped;
    const u32 ua = static_cast<u32>(a);
    const u32 ub = static_cast<u32>(b);
    Character tmpC = memberAt(party, a);                                 // R26.1
    memberAt(party, a) = memberAt(party, b);
    memberAt(party, b) = tmpC;
    CombatOrder tmpO = party.combatOrders[ua];                           // R26.1 standing orders
    party.combatOrders[ua] = party.combatOrders[ub];
    party.combatOrders[ub] = tmpO;
    if (party.selectedIndex == a) {                                      // R26.2
        party.selectedIndex = b;
    } else if (party.selectedIndex == b) {
        party.selectedIndex = a;
    }
    if (a == kLeaderIndex || b == kLeaderIndex) {                        // R26.3
        // UNKNOWN: the NowLeader text is not given in the SRD; wording assumed.
        ctx.say("party.swap.leader", "\"" + memberAt(party, 0).name + "\" is now the party leader.", {}, {});
    }
    return PartySwapOutcome::Swapped;
}

// R27.1, R27.2: the next command index after the selected one, wrapping.
i32 nextCommandIndex(const Party& party) {
    if (party.size <= 0) return 0;
    return (party.selectedIndex + 1) % party.size;
}

// R27.3, R27.4: switch the commanded character.
// UNKNOWN: tap and full views are not modelled here; both reset the Spell Type box.
SwitchResult switchCommandedCharacter(Ctx& ctx, Party& party, i32 targetIndex,
                                      SwitchKind kind, bool inTown) {
    (void)ctx;
    (void)kind;
    SwitchResult r;
    if (targetIndex < 0 || targetIndex >= party.size) return r;
    if (targetIndex == party.selectedIndex) return r;                    // R27.4 nothing to do
    party.selectedIndex = targetIndex;
    r.changed = true;
    r.resetSpellType = true;                                             // R27.4
    // R27.4: in town (leader alive, level 0) Store, Confinement, Morgue, Guild and Bank update.
    // RESOLVED-BY-REFERENCE: R6 (ch16): "If the leader is not in the dungeon (`DungeonLevel <= 0`)
    // and not dead, UPDATE goes to Store, Confinement, Morgue, Guild, Bank, in that order."
    // The level test is <= 0, not == 0. The Seer receives nothing (R6.2), so it is not refreshed.
    r.townRefresh = inTown && party.size > 0 && !memberAt(party, 0).isDead() && party.dungeonLevel <= 0;
    return r;
}

// R28: Alt+1..4 switch command when the party has more than one member and the slot exists.
bool altDigitSwitchesCommand(const Party& party, i32 digit) {
    return party.size > 1 && digit >= 1 && digit <= kCommandKeySlots && digit <= party.size;
}

// R29: at room entry, when a hostile group has engaged, members with no order take their
// Combat Setting (dead or alive). UNKNOWN: the Combat Setting is read as the character's order.
void applyRoomEntryCombatSettings(Ctx& ctx, Party& party, bool hostileEngaged) {
    (void)ctx;
    if (!hostileEngaged) return;                                         // R29.2
    for (i32 i = 0; i < party.size; ++i) {
        CombatOrder& slot = party.combatOrders[static_cast<u32>(i)];
        if (slot.kind == CombatOrderKind::None) {
            slot = memberAt(party, i).order;                             // R29.1
        }
    }
}

// R30.1: city Pickup. UNKNOWN: the refusal state is read from the carrier.
PickupOutcome pickUpBody(Ctx& ctx, Party& party, Character& carrier, Character& target,
                         bool targetHidden) {
    if (carrier.isDead() || carrier.ailments.paralysed) return PickupOutcome::RefusedSilent;
    if (targetHidden) {                                                  // refused before any put-down
        ctx.say("party.pickup.hidden", "\"" + target.name + "\" is hidden!", {}, {});
        return PickupOutcome::RefusedHidden;
    }
    const bool wasCarrying = !carrier.carryingCharacterName.empty();
    if (wasCarrying) {                                                   // R30.1 put down first
        ctx.say("party.pickup.putdown",
                "Put down \"" + carrier.carryingCharacterName + "\".", {}, {});
    }
    carrier.carryingCharacterName = target.name;
    party.carryingSlot = CarryingSlot::DeadCharacter;
    return wasCarrying ? PickupOutcome::PutDownThenPicked : PickupOutcome::Picked;
}

// R30.2: Drop Body. Nothing carried does nothing; refuses silently only when paralysed.
BodyDropOutcome dropBody(Ctx& ctx, Character& carrier) {
    (void)ctx;
    if (carrier.carryingCharacterName.empty()) return BodyDropOutcome::NothingCarried;
    if (carrier.ailments.paralysed) return BodyDropOutcome::RefusedSilent;
    carrier.carryingCharacterName = Str{};
    return BodyDropOutcome::Dropped;
}

// R30.3: Morgue Grab Body. No Hide test, and the current carry is not put down.
// UNKNOWN: the selected member is the carrier, and the grab replaces any current carry name.
bool morgueGrabBody(Ctx& ctx, Party& party, Character& target) {
    (void)ctx;
    if (party.size <= 0) return false;
    i32 idx = party.selectedIndex;
    if (idx < 0 || idx >= party.size) idx = 0;
    Character& carrier = memberAt(party, idx);
    carrier.carryingCharacterName = target.name;
    party.carryingSlot = CarryingSlot::DeadCharacter;
    return true;
}

// R19.6 / R13: CompanionCarry.Run. UNKNOWN: the routine is not specified in this SRD part;
// it is a no-op placeholder here.
void companionCarryRun(Ctx& ctx, Party& party, Vec<Companion>& companions) {
    (void)ctx;
    (void)party;
    (void)companions;
}

}  // namespace mordor
