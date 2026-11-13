#include "test_support.h"
#include "mordor/partyb.h"
#include "mordor/partya.h"

using namespace mordor;

namespace {

Party partyOf(i32 n) {
    Party p;
    p.size = n;
    return p;
}

}  // namespace

// R2, R7: Good and Evil are opposed; Neutral travels with either.
TEST(partya_opposite_alignment_pairs) {
    CHECK(oppositelyAligned(Align::Good, Align::Evil));
    CHECK(oppositelyAligned(Align::Evil, Align::Good));
    CHECK(!oppositelyAligned(Align::Good, Align::Neutral));
    CHECK(!oppositelyAligned(Align::Neutral, Align::Evil));
    CHECK(!oppositelyAligned(Align::Good, Align::Good));
}

// R1, R4.3: the party holds at most 4; full means size >= 4.
TEST(partya_party_full_boundary) {
    CHECK(!isPartyFull(partyOf(3)));
    CHECK(isPartyFull(partyOf(kPartyMaxSize)));
    CHECK_EQ(kPartyMaxSize, 4);
}

// R4.1: a dead first member refuses the join silently.
TEST(partya_join_refused_when_leader_dead) {
    tsup::Env env({0.5, 0.9});
    Party p = partyOf(2);
    p.members[0].deathState = DeathState::Dead;
    Character joiner;
    JoinCheck r = checkJoin(env.ctx, p, joiner, false, false, true);
    CHECK(r.refusal == JoinRefusal::LeaderDead);
}

// R4.2: a dead joiner in a dungeon fight is refused (checked before R8).
TEST(partya_dead_joiner_refused_in_dungeon_combat) {
    tsup::Env env({0.5, 0.9});
    Party p = partyOf(1);
    Character joiner;
    joiner.deathState = DeathState::Dead;
    joiner.pendingRaise = true;
    JoinCheck r = checkJoin(env.ctx, p, joiner, true, true, true);
    CHECK(r.refusal == JoinRefusal::DeadInCombat);
}

// R4.2: a living joiner in a dungeon fight is not refused and runs sub0BDE.
TEST(partya_living_joiner_in_dungeon_fight_runs_sub0bde) {
    tsup::Env env({0.5, 0.9});
    Party p = partyOf(1);
    Character joiner;
    joiner.vitals.curHits = 10;
    JoinCheck r = checkJoin(env.ctx, p, joiner, true, true, true);
    CHECK(r.refusal == JoinRefusal::None);
    CHECK(r.runSub0BDE);
}

// R4.3 and R6: a full party and a wrong password each refuse the join.
TEST(partya_join_full_party_and_wrong_password) {
    tsup::Env env({0.5, 0.9});
    Character joiner;
    joiner.password = "secret";   // R6: a password is set and must match
    Party full = partyOf(kPartyMaxSize);
    CHECK(checkJoin(env.ctx, full, joiner, false, false, true).refusal ==
          JoinRefusal::PartyFull);
    Party one = partyOf(1);
    CHECK(checkJoin(env.ctx, one, joiner, false, false, false).refusal ==
          JoinRefusal::WrongPassword);
}

// R7: a joiner oppositely aligned to a member is refused.
TEST(partya_join_refused_for_opposite_alignment) {
    tsup::Env env({0.5, 0.9});
    Party p = partyOf(1);
    p.members[0].alignment = Align::Good;
    Character joiner;
    joiner.alignment = Align::Evil;
    CHECK(checkJoin(env.ctx, p, joiner, false, false, true).refusal ==
          JoinRefusal::OppositeAlignment);
}

// R8: a dead joiner is refused without a pending raise, admitted with one.
TEST(partya_dead_joiner_needs_pending_raise) {
    tsup::Env env({0.5, 0.9});
    Party p = partyOf(1);
    Character joiner;
    joiner.deathState = DeathState::Dead;
    CHECK(checkJoin(env.ctx, p, joiner, false, false, true).refusal ==
          JoinRefusal::DeadNoPendingRaise);
    joiner.pendingRaise = true;
    CHECK(checkJoin(env.ctx, p, joiner, false, false, true).refusal ==
          JoinRefusal::None);
}

// R7: any member opposed to the joiner makes hasOppositeMember true.
// Neutral members and a Neutral joiner never count as opposed.
TEST(partya_has_opposite_member) {
    Party p = partyOf(2);
    p.members[0].alignment = Align::Good;
    p.members[1].alignment = Align::Neutral;
    CHECK(hasOppositeMember(p, Align::Evil));
    CHECK(!hasOppositeMember(p, Align::Good));
    CHECK(!hasOppositeMember(p, Align::Neutral));
}

// R13: teleporter and chute prune only when the party size dropped.
TEST(partya_prune_on_size_drop) {
    CHECK(pruneOnSizeDrop(3, 2));
    CHECK(!pruneOnSizeDrop(2, 2));
    CHECK(!pruneOnSizeDrop(2, 3));
}

// R10.1, R10.2, R10.3: refusals and the no-effect-until-confirmed rule.
TEST(partya_leave_refusals_and_confirmation) {
    tsup::Env env({0.5, 0.9});
    Party p = partyOf(3);
    LeaveResult r = leaveParty(env.ctx, p, 0, false, false, true);
    CHECK(r.outcome == LeaveOutcome::RefusedLeader);
    r = leaveParty(env.ctx, p, 1, true, true, true);
    CHECK(r.outcome == LeaveOutcome::RefusedMonstersPresent);
    r = leaveParty(env.ctx, p, 1, false, false, false);
    CHECK(r.outcome == LeaveOutcome::NotConfirmed);
    CHECK_EQ(p.size, 3);
}

// R10.3: a confirmed leave removes the member and saves it at the party's
// current level and square.
TEST(partya_leave_saves_leaver_at_party_square) {
    tsup::Env env({0.5, 0.9});
    Party p = partyOf(3);
    // R12: living members need hits > 0 or pruning removes them.
    for (i32 i = 0; i < p.size; ++i) p.members[i].vitals.curHits = 15;
    p.x = 5;
    p.y = 7;
    p.dungeonLevel = 3;
    LeaveResult r = leaveParty(env.ctx, p, 2, true, false, true);
    CHECK(r.outcome == LeaveOutcome::Left);
    CHECK_EQ(r.leaver.location.x, 5);
    CHECK_EQ(r.leaver.location.y, 7);
    CHECK_EQ(r.leaver.location.dungeonLevel, 3);
    CHECK_EQ(p.size, 2);
}

// R11: a lone dead member stays; a living member is never pruned.
TEST(partya_prune_keeps_lone_dead_and_living_members) {
    tsup::Env env({0.5, 0.9});
    Party lone = partyOf(1);
    lone.members[0].deathState = DeathState::Dead;
    (void)pruneOneMember(env.ctx, lone, 0);
    CHECK_EQ(lone.size, 1);

    Party trio = partyOf(3);
    trio.members[1].vitals.curHits = 5;
    (void)pruneOneMember(env.ctx, trio, 1);
    CHECK_EQ(trio.size, 3);
}

// R12: a stoned member with hits above 0 is pruned, and the survivors are
// moved to the leader's X and Y.
TEST(partya_prune_stoned_member_and_move_survivors) {
    tsup::Env env({0.5, 0.9});
    Party p = partyOf(3);
    p.members[0].location.x = 4;
    p.members[0].location.y = 6;
    p.members[0].location.dungeonLevel = 2;
    p.members[1].deathState = DeathState::Stoned;
    p.members[1].vitals.curHits = 5;
    (void)pruneOneMember(env.ctx, p, 1);
    CHECK_EQ(p.size, 2);
    for (i32 i = 0; i < p.size; ++i) {
        CHECK_EQ(p.members[i].location.x, 4);
        CHECK_EQ(p.members[i].location.y, 6);
    }
}

// R14: the UPDATE fires only when some member is dead.
TEST(partya_update_fires_only_with_dead_member) {
    tsup::Env env({0.5, 0.9});
    Party p = partyOf(2);
    CHECK(!firePartyUpdateIfDead(env.ctx, p));
    p.members[1].deathState = DeathState::Dead;
    CHECK(firePartyUpdateIfDead(env.ctx, p));
}

// R17: one member left stops the group; a lone death ends the monster turn.
TEST(partya_group_turn_after_knockout) {
    tsup::Env env({0.5, 0.9});
    Vec<Companion> companions;

    // R12: living members need hits > 0 to survive pruning; dead ones have 0 hits.
    Party three = partyOf(3);
    for (i32 i = 0; i < three.size; ++i) three.members[i].vitals.curHits = 15;
    three.members[1].deathState = DeathState::Dead;
    three.members[1].vitals.curHits = 0;
    CHECK(afterTargetKnockedOut(env.ctx, three, companions, 1) ==
          GroupTurnStep::Continue);

    Party two = partyOf(2);
    for (i32 i = 0; i < two.size; ++i) two.members[i].vitals.curHits = 15;
    two.members[1].deathState = DeathState::Dead;
    two.members[1].vitals.curHits = 0;
    CHECK(afterTargetKnockedOut(env.ctx, two, companions, 1) ==
          GroupTurnStep::GroupStops);

    Party solo = partyOf(1);
    solo.members[0].deathState = DeathState::Dead;
    solo.members[0].vitals.curHits = 0;
    CHECK(afterTargetKnockedOut(env.ctx, solo, companions, 0) ==
          GroupTurnStep::MonsterTurnEnds);
}

// R27.1, R27.2: the next commanded index is (selected + 1) mod size; a lone
// character leaves the index unchanged.
TEST(partya_next_command_index_wraps) {
    Party p = partyOf(3);
    p.selectedIndex = 0;
    CHECK_EQ(nextCommandIndex(p), 1);
    p.selectedIndex = 2;
    CHECK_EQ(nextCommandIndex(p), 0);

    Party solo = partyOf(1);
    solo.selectedIndex = 0;
    CHECK_EQ(nextCommandIndex(solo), 0);
}

// R28: Alt+1..4 switch command only when size > 1 and the slot exists.
TEST(partya_alt_digit_needs_size_and_slot) {
    Party p = partyOf(3);
    CHECK(altDigitSwitchesCommand(p, 1));
    CHECK(altDigitSwitchesCommand(p, 3));
    CHECK(!altDigitSwitchesCommand(p, 4));
    CHECK(!altDigitSwitchesCommand(p, 0));
    CHECK(!altDigitSwitchesCommand(partyOf(1), 1));
}

// R26: a swap exchanges members and the selection follows the moved
// character; swaps are refused in a dungeon fight and reject bad indices.
TEST(partya_swap_positions_rules) {
    tsup::Env env({0.5, 0.9});
    Party p = partyOf(3);
    p.members[1].level = 5;
    p.members[2].level = 7;
    p.selectedIndex = 1;
    CHECK(rearrangeParty(env.ctx, p, 1, 2, false) ==
          RearrangeOutcome::Swapped);
    CHECK_EQ(p.members[1].level, 7);
    CHECK_EQ(p.members[2].level, 5);
    CHECK_EQ(p.selectedIndex, 2);
    CHECK(rearrangeParty(env.ctx, p, 0, 9, false) ==
          RearrangeOutcome::OutOfRange);
    p.inDungeon = true;
    CHECK(rearrangeParty(env.ctx, p, 0, 1, true) ==
          RearrangeOutcome::InCombat);
}

// R25.1: input needs a comma, at most 5 characters, and exactly two integers.
TEST(partya_parse_swap_input_format) {
    SwapInputResult ok = parseSwapInput("3,4");
    CHECK(ok.ok);
    CHECK_EQ(ok.first, 3);
    CHECK_EQ(ok.second, 4);

    SwapInputResult five = parseSwapInput("1,234");
    CHECK(five.ok);
    CHECK_EQ(five.second, 234);

    CHECK(!parseSwapInput("34").ok);
    CHECK(!parseSwapInput("1,2,3").ok);
    CHECK(!parseSwapInput("1,2345").ok);
}

// R20 gold answer: zero gives nothing; above on-hand gold is refused, and
// the bank does not count toward a Give.
TEST(partya_give_gold_rules) {
    tsup::Env env({0.5, 0.9});
    // Currency is i64 in 1/10000 gold; Give works in whole gold (currencyWhole).
    Character giver;
    Character receiver;
    giver.wallet.goldOnHand = 500 * 10000;

    CHECK(giveGoldTransfer(env.ctx, giver, receiver, 0) ==
          GiveGoldOutcome::ZeroNoOp);
    CHECK(giveGoldTransfer(env.ctx, giver, receiver, 600 * 10000) ==
          GiveGoldOutcome::NotEnoughGold);
    CHECK_EQ(giver.wallet.goldOnHand, 500 * 10000);

    Character banker;
    banker.wallet.goldInBank = 1000 * 10000;
    CHECK(giveGoldTransfer(env.ctx, banker, receiver, 1 * 10000) ==
          GiveGoldOutcome::NotEnoughGold);

    CHECK(giveGoldTransfer(env.ctx, giver, receiver, 500 * 10000) ==
          GiveGoldOutcome::Done);
    CHECK_EQ(giver.wallet.goldOnHand, 0);
    CHECK_EQ(receiver.purse(), 500 * 10000);
}

// R23: dungeon exchange uses on-hand gold only; town adds the bank. A town
// transfer draws any on-hand deficit from the bank.
TEST(partya_exchange_available_and_transfer) {
    tsup::Env env({0.5, 0.9});
    Character c;
    c.wallet.goldOnHand = 300;
    c.wallet.goldInBank = 700;
    CHECK_EQ(exchangeAvailable(c, true), 300);
    CHECK_EQ(exchangeAvailable(c, false), 1000);

    Character from = c;
    Character to;
    CHECK(transferExchangeGold(env.ctx, from, to, 0, true) ==
          GoldTransferOutcome::ZeroNoOp);
    CHECK(transferExchangeGold(env.ctx, from, to, 301, true) ==
          GoldTransferOutcome::TooMuch);
    CHECK_EQ(from.purse(), 1000);
    CHECK(transferExchangeGold(env.ctx, from, to, 100, true) ==
          GoldTransferOutcome::Done);
    CHECK_EQ(from.wallet.goldOnHand, 200);
    CHECK_EQ(to.purse(), 100);

    Character townFrom;
    townFrom.wallet.goldOnHand = 100;
    townFrom.wallet.goldInBank = 900;
    Character townTo;
    CHECK(transferExchangeGold(env.ctx, townFrom, townTo, 400, false) ==
          GoldTransferOutcome::Done);
    CHECK_EQ(townFrom.wallet.goldOnHand, 0);
    CHECK_EQ(townFrom.wallet.goldInBank, 600);
    CHECK_EQ(townTo.purse(), 400);
}

// R22.1 is checked before R22.2; a two-member party pools when not in combat.
TEST(partya_pool_gold_refusals) {
    tsup::Env env({0.5, 0.9});
    Party solo = partyOf(1);
    CHECK(poolGold(env.ctx, solo, 0, true) == PoolOutcome::RefusedAlone);

    Party duo = partyOf(2);
    CHECK(poolGold(env.ctx, duo, 0, true) == PoolOutcome::RefusedInCombat);
    CHECK(poolGold(env.ctx, duo, 0, false) == PoolOutcome::Pooled);
}

// R18: a Yes-reload writes the record, a No deletes it; a party of more than
// one asks the question; a dead leader's record is not written; monsters
// standing refuse the disband unless the leader is dead.
TEST(partya_disband_record_and_refusal) {
    tsup::Env env({0.5, 0.9});
    Party duo = partyOf(2);
    CHECK(resolveDisband(duo, true) == RecordAction::WriteRecord);
    CHECK(resolveDisband(duo, false) == RecordAction::DeleteRecord);

    DisbandPlan plan = planDisband(env.ctx, duo, false);
    CHECK(!plan.refused);
    CHECK(plan.askReload);
    CHECK(!plan.leaderDead);

    plan = planDisband(env.ctx, duo, true);
    CHECK(plan.refused);

    duo.members[0].deathState = DeathState::Dead;
    plan = planDisband(env.ctx, duo, true);
    CHECK(!plan.refused);
    CHECK(plan.leaderDead);
}

// R27.4: switching to the commanded character changes nothing; a switch to
// another character resets the Spell Type box and is not a town refresh.
TEST(partya_switch_commanded_character) {
    tsup::Env env({0.5, 0.9});
    Party p = partyOf(3);
    p.selectedIndex = 0;

    SwitchResult same = switchCommandedCharacter(env.ctx, p, 0, SwitchKind::Full, false);
    CHECK(!same.changed);
    CHECK(!same.resetSpellType);

    SwitchResult other = switchCommandedCharacter(env.ctx, p, 2, SwitchKind::Full, false);
    CHECK(other.changed);
    CHECK(other.resetSpellType);
    CHECK(!other.townRefresh);
    CHECK_EQ(p.selectedIndex, 2);
}

