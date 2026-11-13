#include "test_support.h"
#include "mordor/partyb.h"

using namespace mordor;

namespace {

// A character with a name, alignment and dungeon level, living at the given location.
Character makeChar(const char* name, Align al, i32 dungeonLevel) {
    Character c;
    c.name = name;
    c.alignment = al;
    c.location.dungeonLevel = dungeonLevel;
    c.vitals.curHits = 10;
    c.vitals.maxHits = 15;
    return c;
}

// A party whose size is n. Every member slot starts alive so pruning does not remove anyone.
Party makeParty(i32 n) {
    Party p;
    p.size = n;
    for (u32 i = 0; i < static_cast<u32>(kPartyMaxSize); ++i) {
        p.members[i].vitals.curHits = 10;
    }
    return p;
}

i32 count(const Vec<i32>& v) {
    return static_cast<i32>(v.size());
}

}  // namespace

TEST(help_cry_thresholds) {
    Character c;
    c.vitals.maxHits = 100;
    c.vitals.curHits = 9;
    CHECK(helpCryNeeded(c));   // 9 < 10
    c.vitals.curHits = 10;
    CHECK(helpCryNeeded(c));   // 10 is not < 10, but 10 <= 100 * 0.1 = 10
    c.vitals.curHits = 11;
    CHECK(!helpCryNeeded(c));  // 11 > 10 and 11 > 10.0
    c.vitals.maxHits = 200;
    c.vitals.curHits = 20;
    CHECK(helpCryNeeded(c));   // 20 <= 200 * 0.1 = 20
    c.vitals.curHits = 21;
    CHECK(!helpCryNeeded(c));
}

TEST(monster_target_weight_table) {
    CHECK_EQ(kMonsterTargetWeightCount, 8);
    CHECK_EQ(monsterTargetWeight(0), 100);
    CHECK_EQ(monsterTargetWeight(1), 50);
    CHECK_EQ(monsterTargetWeight(2), 25);
    CHECK_EQ(kMonsterTargetWeights[3], 12);
    CHECK_EQ(monsterTargetWeight(7), 0);
}

TEST(pick_monster_target_stays_in_party) {
    // A single member is always target 0, whatever the roll.
    // FIXED: a single-element braced list of doubles narrows to u64; a second roll
    // value (matching the two-element form used by the other tests) avoids that.
    tsup::Env one({0.99, 0.9});
    CHECK_EQ(pickMonsterTarget(one.ctx, 1), 0);

    // With three members the target is clamped to 0..2 at both ends of the roll.
    // FIXED: same narrowing fix as above.
    tsup::Env low({0.0, 0.9});
    tsup::Env high({0.999, 0.9});
    i32 tLow = pickMonsterTarget(low.ctx, 3);
    i32 tHigh = pickMonsterTarget(high.ctx, 3);
    CHECK(tLow >= 0 && tLow <= 2);
    CHECK(tHigh >= 0 && tHigh <= 2);
}

TEST(opposed_alignment_is_good_versus_evil_only) {
    CHECK(opposedAlignment(Align::Good, Align::Evil));
    CHECK(opposedAlignment(Align::Evil, Align::Good));
    CHECK(!opposedAlignment(Align::Good, Align::Neutral));
    CHECK(!opposedAlignment(Align::Neutral, Align::Evil));
    CHECK(!opposedAlignment(Align::Good, Align::Good));
    // The autoload clash agrees with the join rule on these pairs for any enum ordering.
    CHECK(autoloadAlignmentClash(Align::Good, Align::Evil));
    CHECK(!autoloadAlignmentClash(Align::Good, Align::Neutral));
    CHECK(!autoloadAlignmentClash(Align::Neutral, Align::Evil));
    CHECK(!autoloadAlignmentClash(Align::Good, Align::Good));
}

TEST(same_place_town_and_dungeon_rules) {
    Character a = makeChar("A", Align::Good, 0);
    Character b = makeChar("B", Align::Evil, 0);
    a.location.x = 1;
    b.location.x = 5;
    CHECK(samePlace(a, b));  // both in town, coordinates ignored

    a.location.dungeonLevel = 2;
    b.location.dungeonLevel = 2;
    a.location.x = 1;
    a.location.y = 1;
    b.location.x = 1;
    b.location.y = 1;
    CHECK(samePlace(a, b));  // same level and square
    b.location.y = 2;
    CHECK(!samePlace(a, b));  // different square
    b.location.y = 1;
    b.location.dungeonLevel = 3;
    CHECK(!samePlace(a, b));  // different level
    a.location.dungeonLevel = 1;
    b.location.dungeonLevel = 0;
    CHECK(!samePlace(a, b));  // town and dungeon
}

TEST(join_party_refuses_when_full) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(4);
    Character newcomer = makeChar("E", Align::Good, 0);
    CHECK(joinParty(ctx, party, newcomer, false) == PartyJoinOutcome::Full);
    CHECK_EQ(party.size, 4);
}

TEST(join_party_adds_compatible_member_in_same_place) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(1);
    party.members[0] = makeChar("A", Align::Good, 0);
    Character newcomer = makeChar("B", Align::Neutral, 0);
    CHECK(joinParty(ctx, party, newcomer, false) == PartyJoinOutcome::Joined);
    CHECK_EQ(party.size, 2);
}

TEST(join_party_refuses_opposed_and_duplicate) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(1);
    party.members[0] = makeChar("A", Align::Good, 0);

    Character evil = makeChar("B", Align::Evil, 0);
    CHECK(joinParty(ctx, party, evil, false) == PartyJoinOutcome::Opposed);

    Character dup = makeChar("A", Align::Neutral, 0);
    CHECK(joinParty(ctx, party, dup, false) == PartyJoinOutcome::AlreadyInParty);
    CHECK_EQ(party.size, 1);
}

TEST(join_party_refuses_dead_and_not_here) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(1);
    party.members[0] = makeChar("A", Align::Good, 0);

    Character dead = makeChar("B", Align::Neutral, 0);
    dead.deathState = DeathState::Dead;
    CHECK(joinParty(ctx, party, dead, false) == PartyJoinOutcome::Dead);

    Character away = makeChar("C", Align::Neutral, 1);
    CHECK(joinParty(ctx, party, away, false) == PartyJoinOutcome::NotHere);
    CHECK_EQ(party.size, 1);
}

TEST(here_to_join_needs_level_floor_area_and_no_party) {
    Party none = makeParty(0);
    Character a = makeChar("A", Align::Good, 2);
    a.location.area = 5;
    Character b = makeChar("B", Align::Good, 0);
    b.location.area = 5;
    Character c = makeChar("C", Align::Good, 3);
    c.location.area = 5;
    Character d = makeChar("D", Align::Good, 2);
    d.location.area = 7;
    Vec<Character> roster{a, b, c, d};

    // Only A matches: level 2, within 2 floors, area 5.
    CHECK_EQ(count(hereToJoinCandidates(none, roster, 2, 2, 5)), 1);
    // Level 2 is above a 1-floor dungeon.
    CHECK_EQ(count(hereToJoinCandidates(none, roster, 2, 1, 5)), 0);
    // C is on level 3 which is beyond 2 floors.
    CHECK_EQ(count(hereToJoinCandidates(none, roster, 3, 2, 5)), 0);

    // A is already in the party, so it is not a candidate.
    Party withA = makeParty(1);
    withA.members[0] = makeChar("A", Align::Good, 0);
    CHECK_EQ(count(hereToJoinCandidates(withA, roster, 2, 2, 5)), 0);
}

TEST(inherit_lost_flags_copies_from_first) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Character first;
    first.lost.position = false;
    first.lost.level = false;
    first.lost.facing = true;
    Character newcomer;
    newcomer.lost.position = true;
    newcomer.lost.level = true;
    newcomer.lost.facing = false;
    inheritLostFlags(ctx, first, newcomer);
    CHECK(!newcomer.lost.position);
    CHECK(!newcomer.lost.level);
    CHECK(newcomer.lost.facing);
}

TEST(pool_gold_to_selected_sums_onto_selected) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(3);
    party.selectedIndex = 1;
    party.members[0].wallet.goldOnHand = 10;
    party.members[1].wallet.goldOnHand = 20;
    party.members[2].wallet.goldOnHand = 30;
    poolGoldToSelected(ctx, party);
    CHECK_EQ(party.members[1].wallet.goldOnHand, 60);
    CHECK_EQ(party.members[0].wallet.goldOnHand, 0);
    CHECK_EQ(party.members[2].wallet.goldOnHand, 0);
}

TEST(pool_overflow_stops_loop_silently) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(3);
    party.selectedIndex = 0;
    // Selected holds INT64_MAX - 7; adding member 1's 10 overflows.
    party.members[0].wallet.goldOnHand = 9223372036854775800LL;
    party.members[1].wallet.goldOnHand = 10;
    party.members[2].wallet.goldOnHand = 7;
    CHECK(poolPartyGold(ctx, party, PartyPoolKind::ToSelected, false) == PartyPoolOutcome::OverflowStopped);
    // The loop stopped before member 2, so its gold is still on hand.
    CHECK_EQ(party.members[2].wallet.goldOnHand, 7);
}

TEST(pool_party_gold_refusals) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party alone = makeParty(1);
    CHECK(poolPartyGold(ctx, alone, PartyPoolKind::ToSelected, false) == PartyPoolOutcome::NobodyToPoolTo);

    Party pair = makeParty(2);
    CHECK(poolPartyGold(ctx, pair, PartyPoolKind::ToSelected, true) == PartyPoolOutcome::InCombat);
}

TEST(pool_to_bank_and_party_deposit_rules) {
    // PoolToBank: every member's on-hand gold goes to Selected's bank, Selected included.
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party pool = makeParty(2);
    pool.selectedIndex = 0;
    pool.members[0].wallet.goldOnHand = 5;
    pool.members[0].wallet.goldInBank = 100;
    pool.members[1].wallet.goldOnHand = 7;
    pool.members[1].wallet.goldInBank = 1;
    poolGoldToBank(ctx, pool);
    CHECK_EQ(pool.members[0].wallet.goldInBank, 112);
    CHECK_EQ(pool.members[0].wallet.goldOnHand, 0);
    CHECK_EQ(pool.members[1].wallet.goldOnHand, 0);
    CHECK_EQ(pool.members[1].wallet.goldInBank, 1);

    // PartyDeposit: each member's on-hand gold goes into their own bank.
    Party dep = makeParty(2);
    dep.members[0].wallet.goldOnHand = 5;
    dep.members[0].wallet.goldInBank = 1;
    dep.members[1].wallet.goldOnHand = 0;
    dep.members[1].wallet.goldInBank = 3;
    partyDepositGold(ctx, dep);
    CHECK_EQ(dep.members[0].wallet.goldInBank, 6);
    CHECK_EQ(dep.members[0].wallet.goldOnHand, 0);
    CHECK_EQ(dep.members[1].wallet.goldInBank, 3);
}

TEST(pool_and_deposit_worked_example) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(3);
    party.selectedIndex = 0;
    party.members[0].wallet.goldOnHand = 4;
    party.members[0].wallet.goldInBank = 10;
    party.members[1].wallet.goldOnHand = 3;
    party.members[1].wallet.goldInBank = 20;
    party.members[2].wallet.goldOnHand = 0;
    party.members[2].wallet.goldInBank = 5;
    poolAndDepositGold(ctx, party);
    // 10 + (20 + 3) + (5 + 0) + 4 = 42
    CHECK_EQ(party.members[0].wallet.goldInBank, 42);
    CHECK_EQ(party.members[0].wallet.goldOnHand, 0);
    CHECK_EQ(party.members[1].wallet.goldInBank, 0);
    CHECK_EQ(party.members[1].wallet.goldOnHand, 0);
    CHECK_EQ(party.members[2].wallet.goldInBank, 0);
    CHECK_EQ(party.members[2].wallet.goldOnHand, 0);
}

TEST(transfer_gold_dungeon_uses_on_hand_only) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Character from;
    from.wallet.goldOnHand = 50;
    from.wallet.goldInBank = 100;
    Character to;

    // In the dungeon only the 50 on hand is available, so 60 is "Too Much!".
    transferGold(ctx, from, to, 60, true);
    CHECK_EQ(from.wallet.goldOnHand, 50);
    CHECK_EQ(from.wallet.goldInBank, 100);
    CHECK_EQ(to.wallet.goldOnHand, 0);

    transferGold(ctx, from, to, 0, false);  // zero amount does nothing
    CHECK_EQ(to.wallet.goldOnHand, 0);

    // In town the purse (150) is available. The bank covers the shortfall.
    transferGold(ctx, from, to, 60, false);
    CHECK_EQ(to.wallet.goldOnHand, 60);
    CHECK_EQ(from.wallet.goldOnHand, 0);
    CHECK_EQ(from.wallet.goldInBank, 90);
}

TEST(transfer_gold_negative_amount_quirk) {
    // Documented quirk: a negative amount runs backwards with no check,
    // so the recipient's gold goes below zero.
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Character from;
    from.wallet.goldOnHand = 10;
    Character to;
    to.wallet.goldOnHand = 0;
    transferGold(ctx, from, to, -4, false);
    CHECK_EQ(to.wallet.goldOnHand, -4);
    CHECK_EQ(from.wallet.goldOnHand, 14);
}

TEST(remove_member_rules_and_selected_clamp) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(3);
    party.members[0].name = "A";
    party.members[1].name = "B";
    party.members[2].name = "C";
    party.selectedIndex = 2;

    CHECK(!removeMember(ctx, party, Str("Z")));  // not in party
    CHECK(removeMember(ctx, party, Str("B")));
    CHECK_EQ(party.size, 2);
    CHECK_EQ(party.selectedIndex, 1);  // 2 clamped to Count - 1

    Party solo = makeParty(1);
    solo.members[0].name = "A";
    CHECK(!removeMember(ctx, solo, Str("A")));  // a lone member cannot be removed
    CHECK_EQ(solo.size, 1);
}

TEST(rearrange_swaps_only_the_two_slots) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(4);
    party.members[0].name = "P0";
    party.members[1].name = "P1";
    party.members[2].name = "P2";
    party.members[3].name = "P3";
    party.selectedIndex = 1;

    // Swapping 1 and 3 must leave 2 where it is.
    CHECK(rearrangeParty(ctx, party, 1, 3, false) == RearrangeOutcome::Swapped);
    CHECK(party.members[1].name == Str("P3"));
    CHECK(party.members[2].name == Str("P2"));
    CHECK(party.members[3].name == Str("P1"));
    CHECK_EQ(party.selectedIndex, 3);  // the selected index follows the moved member

    CHECK(rearrangeParty(ctx, party, 0, 4, false) == RearrangeOutcome::OutOfRange);
    CHECK(rearrangeParty(ctx, party, 2, 2, false) == RearrangeOutcome::SameSlot);
}

TEST(select_next_able_skips_dead_and_wraps) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(3);
    party.members[1].deathState = DeathState::Dead;

    // Wrap around from slot 2 to slot 0, which is able.
    party.selectedIndex = 2;
    selectNextAble(ctx, party);
    CHECK_EQ(party.selectedIndex, 0);

    // From slot 0 the next slot is dead, so the search lands on slot 2.
    party.selectedIndex = 0;
    selectNextAble(ctx, party);
    CHECK_EQ(party.selectedIndex, 2);
}

TEST(cycle_selected_member_wraps_forward) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(3);
    party.selectedIndex = 2;
    cycleSelectedMember(ctx, party);
    CHECK_EQ(party.selectedIndex, 0);
    cycleSelectedMember(ctx, party);
    CHECK_EQ(party.selectedIndex, 1);
}

TEST(combat_setting_mapping) {
    CHECK(combatSettingKind(0) == CombatOrderKind::None);
    CHECK(combatSettingKind(1) == CombatOrderKind::Fight);
    CHECK(combatSettingKind(2) == CombatOrderKind::Defend);
    CHECK(combatSettingKind(3) == CombatOrderKind::Buffer);
    CHECK(combatSettingKind(12) == CombatOrderKind::Buffer);  // 3 + 9
    CHECK_EQ(combatSettingBufferIndex(3), 0);
    CHECK_EQ(combatSettingBufferIndex(12), 9);
    CHECK_EQ(combatSettingFromKind(CombatOrderKind::Fight, 0), 1);
    CHECK_EQ(combatSettingFromKind(CombatOrderKind::Defend, 0), 2);
    CHECK_EQ(combatSettingFromKind(CombatOrderKind::Buffer, 9), 12);
}

TEST(party_status_code_letters) {
    Character c;
    CHECK(partyStatusCode(c) == Str("OK"));
    c.ailments.poisonLevel = 1;
    CHECK(partyStatusCode(c) == Str("Po"));
    c.ailments.diseaseLevel = 2;
    CHECK(partyStatusCode(c) == Str("PoDi"));
    c.ailments.paralysed = true;
    CHECK(partyStatusCode(c) == Str("PoDiPa"));

    Character d;
    d.ailments.paralysed = true;
    CHECK(partyStatusCode(d) == Str("Pa"));
}

TEST(give_gold_and_item_refusals) {
    // Currency is i64 in 1/10000 gold, so amounts are written with currencyFromWhole.
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(2);
    Character& giver = party.members[0];
    giver.wallet.goldOnHand = currencyFromWhole(100);

    CHECK(giveGoldToMember(ctx, party, giver, 1, 0) == PartyGiveGoldOutcome::NothingToGive);
    CHECK(giveGoldToMember(ctx, party, giver, 1, currencyFromWhole(-3)) == PartyGiveGoldOutcome::NothingToGive);
    CHECK(giveGoldToMember(ctx, party, giver, 1, currencyFromWhole(101)) == PartyGiveGoldOutcome::NotEnoughGold);
    CHECK_EQ(giver.wallet.goldOnHand, currencyFromWhole(100));

    // Exactly the amount on hand is allowed.
    CHECK(giveGoldToMember(ctx, party, giver, 1, currencyFromWhole(100)) == PartyGiveGoldOutcome::Given);
    CHECK_EQ(giver.wallet.goldOnHand, currencyFromWhole(0));
    CHECK_EQ(party.members[1].wallet.goldOnHand, currencyFromWhole(100));

    // Pack slot 0 is the Hands slot, so there is no item to give.
    CHECK(giveItemToMember(ctx, party, giver, 1, 0) == PartyGiveItemOutcome::NoItemToGive);
}

TEST(item_slot_index_formula) {
    CHECK_EQ(itemSlotIndex(0), 0);
    CHECK_EQ(itemSlotIndex(1), 0);
    CHECK_EQ(itemSlotIndex(2), 1);
    CHECK_EQ(itemSlotIndex(40), 39);
}

TEST(leave_party_refusals_and_success) {
    tsup::Env env({0.5, 0.9});
    Ctx& ctx = env.ctx;
    Party party = makeParty(2);

    party.selectedIndex = 0;
    CHECK(leaveParty(ctx, party, true, false) == PartyLeaveOutcome::LeaderCannotLeave);

    party.selectedIndex = 1;
    CHECK(leaveParty(ctx, party, true, true) == PartyLeaveOutcome::MonstersInArea);
    CHECK(leaveParty(ctx, party, false, false) == PartyLeaveOutcome::NotConfirmed);
    CHECK_EQ(party.size, 2);

    CHECK(leaveParty(ctx, party, true, false) == PartyLeaveOutcome::Left);
    CHECK_EQ(party.size, 1);
}
