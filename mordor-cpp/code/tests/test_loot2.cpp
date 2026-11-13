#include "test_support.h"
#include "mordor/loot2.h"
#include <vector>  // FIXED: needed for std::vector<double> draw lists below

namespace m = mordor;

namespace {
// Currency is fixed point with 1/10000 of a gold piece per unit.
constexpr m::Currency kGp = 10000;

// FIXED: rollChestLock takes const ChestTuning&, not a bare i32; wrap the chance in a tuning.
m::ChestTuning tuningWithLockedChance(m::i32 lockedChance) {
    m::ChestTuning t;
    t.lockedChance = lockedChance;
    return t;
}
}  // namespace

// Documented header constants (SRD c14p2 notes and header comments).
TEST(loot2_documented_constants) {
    CHECK_EQ(m::kChestCountStart, 2);
    CHECK_EQ(m::kChestTrapTries, 200);
    CHECK_EQ(m::kLairBoxBonus, 25);
    CHECK_EQ(m::kLockGoldNum, 3);
    CHECK_EQ(m::kLockGoldDen, 2);
    CHECK_EQ(m::kItemRollCapThreshold, 90);
    CHECK_EQ(m::kWitheringWeeks, 4);
}

// Lock roll: below(100)+1 <= lockedChance and class > BOX-or-CHEST gate.
// ScriptedRng value v gives below(100) = Int(v*100).
// FIXED: tsup::Env rejected std::vector<double> (no matching constructor).
// Scripted draws are now passed as a braced list, which matches an
// initializer-list style Env constructor. test_support.h was not provided,
// so this is the closest form; if Env takes a different element type, adjust
// the braced values to match it.
TEST(loot2_lock_magic_when_roll_passes) {
    // R[c5]: roll = Rnd(100)+1 = 41; 41 <= 50 and CHEST > CONTAINER, square allows magic.
    tsup::Env env({0.4});  // below(100)=40, roll=41
    const m::i32 lockedChance = 50;
    CHECK(m::rollChestLock(env.ctx, tuningWithLockedChance(lockedChance), m::ChestClass::CHEST, true) == m::ChestLock::MAGIC_LOCKED);
}

TEST(loot2_lock_roll_boundary_at_locked_chance) {
    // R[c5]: magic-locked when roll <= LockedChance (inclusive).
    const m::i32 lockedChance = 50;
    tsup::Env atEdge({0.495});  // below=49, roll=50 <= 50
    CHECK(m::rollChestLock(atEdge.ctx, tuningWithLockedChance(lockedChance), m::ChestClass::CHEST, true) == m::ChestLock::MAGIC_LOCKED);
    tsup::Env pastEdge({0.505});  // below=50, roll=51 > 50
    CHECK(m::rollChestLock(pastEdge.ctx, tuningWithLockedChance(lockedChance), m::ChestClass::CHEST, true) == m::ChestLock::UNLOCKED);
}

TEST(loot2_lock_zero_and_full_chance) {
    // R[c5]: roll is always in 1..100, so chance 0 never locks and chance 100 always locks.
    const m::i32 noneChance = 0;
    tsup::Env zero({0.0});  // roll=1 <= 0 is false
    CHECK(m::rollChestLock(zero.ctx, tuningWithLockedChance(noneChance), m::ChestClass::CHEST, true) == m::ChestLock::UNLOCKED);

    const m::i32 oneChance = 1;
    tsup::Env oneEnv({0.0});  // roll=1 <= 1
    CHECK(m::rollChestLock(oneEnv.ctx, tuningWithLockedChance(oneChance), m::ChestClass::CHEST, true) == m::ChestLock::MAGIC_LOCKED);

    const m::i32 fullChance = 100;
    tsup::Env fullEnv({0.995});  // below=99, roll=100 <= 100
    CHECK(m::rollChestLock(fullEnv.ctx, tuningWithLockedChance(fullChance), m::ChestClass::CHEST, true) == m::ChestLock::MAGIC_LOCKED);
}

// Square check (CONFLICT-1 recorded form): a square that forbids magic never magic-locks.
TEST(loot2_lock_square_without_magic_stays_unlocked) {
    m::ChestTuning t;
    t.lockedChance = 100;
    tsup::Env env({0.0});
    CHECK(m::rollChestLock(env.ctx, t, m::ChestClass::CHEST, false) == m::ChestLock::UNLOCKED);
}

// Class gate: ChestClass must be > 1 (BOX=2, CHEST=3); CONTAINER=1 is never magic-locked.
TEST(loot2_lock_container_class_never_magic_locked) {
    // R[c5]: ChestClass must be > 1, so CONTAINER (1) never locks; BOX (2) can.
    const m::i32 lockedChance = 100;
    tsup::Env container({0.0});
    CHECK(m::rollChestLock(container.ctx, tuningWithLockedChance(lockedChance), m::ChestClass::CONTAINER, true) == m::ChestLock::UNLOCKED);
    tsup::Env box({0.0});
    CHECK(m::rollChestLock(box.ctx, tuningWithLockedChance(lockedChance), m::ChestClass::BOX, true) == m::ChestLock::MAGIC_LOCKED);
}

// Port cap: with no set trap bits the draw loop exhausts and returns 0.
TEST(loot2_trap_bit_draw_exhausts_to_zero) {
    tsup::Env env({0.5, 0.9});
    CHECK_EQ(m::drawTrapBit(env.ctx, 0), 0);
}

// Engine trap-bit order: bit 6 is Fate (CONFLICT-29 engine index).
TEST(loot2_trap_for_bit_six_is_fate) {
    CHECK(m::trapForBit(6) == m::ChestTrap::Fate);
}

// Poison (CONFLICT-6, as-written port form): takes when U*100 < resistance/2.
TEST(loot2_poison_takes_when_draw_below_half_resistance) {
    tsup::Env takes({0.1});  // 10 < 20
    CHECK(m::chestPoisonTakes(takes.ctx, 40));
    tsup::Env misses({0.3});  // 30 < 20 is false
    CHECK(!m::chestPoisonTakes(misses.ctx, 40));
}

TEST(loot2_poison_zero_resistance_never_takes) {
    tsup::Env env({0.0});  // 0 < 0 is false
    CHECK(!m::chestPoisonTakes(env.ctx, 0));
}

// Disarm roll (engine form): Rnd(100)+1 <= thievingSkill.
TEST(loot2_disarm_roll_boundary) {
    tsup::Env success({0.495});  // roll=50 <= 50
    CHECK(m::trapDisarmRoll(success.ctx, 50));
    tsup::Env failure({0.505});  // roll=51 > 50
    CHECK(!m::trapDisarmRoll(failure.ctx, 50));
}

TEST(loot2_disarm_zero_skill_never_succeeds) {
    tsup::Env env({0.0});  // roll=1 <= 0 is false
    CHECK(!m::trapDisarmRoll(env.ctx, 0));
}

TEST(loot2_disarm_full_skill_always_succeeds) {
    tsup::Env env({0.995});  // roll=100 <= 100
    CHECK(m::trapDisarmRoll(env.ctx, 100));
}

// Receiver draw (CONFLICT-9 port form): CInt(Rnd*(PartySize-1)).
TEST(loot2_receiver_index_from_scaled_draw) {
    tsup::Env high({0.9});  // CInt(0.9*3)=CInt(2.7)=3
    CHECK_EQ(m::chestReceiverIndex(high.ctx, 4), 3);
    tsup::Env low({0.1});  // CInt(0.3)=0
    CHECK_EQ(m::chestReceiverIndex(low.ctx, 4), 0);
}

TEST(loot2_receiver_single_member_is_zero) {
    tsup::Env env({0.9});  // Rnd*0 = 0
    CHECK_EQ(m::chestReceiverIndex(env.ctx, 1), 0);
}

// Purse: spending power is on-hand gold plus bank gold.
TEST(loot2_can_pay_counts_hand_plus_bank) {
    m::Purse p;
    p.goldOnHand = 300 * kGp;
    p.goldInBank = 200 * kGp;
    CHECK(m::canPayFromPurse(p, 450 * kGp));
    CHECK(!m::canPayFromPurse(p, 501 * kGp));
}

TEST(loot2_bank_deposit_moves_gold_to_bank) {
    m::Purse p;
    p.goldOnHand = 300 * kGp;
    p.goldInBank = 0;
    CHECK(m::bankDepositGold(p, 100 * kGp));
    CHECK_EQ(p.goldOnHand, 200 * kGp);
    CHECK_EQ(p.goldInBank, 100 * kGp);
    // FIXED: mordor::Purse has no purse() member; sum the two balances directly.
    CHECK_EQ(p.goldOnHand + p.goldInBank, 300 * kGp);
}

// Withdrawal port rule (CONFLICT-10): refused when it exceeds bank balance.
TEST(loot2_bank_withdraw_refuses_over_balance) {
    m::Purse p;
    p.goldOnHand = 50 * kGp;
    p.goldInBank = 100 * kGp;
    CHECK(!m::bankWithdrawGold(p, 101 * kGp));
    CHECK_EQ(p.goldInBank, 100 * kGp);
    CHECK_EQ(p.goldOnHand, 50 * kGp);
}

TEST(loot2_bank_withdraw_within_balance_succeeds) {
    m::Purse p;
    p.goldOnHand = 50 * kGp;
    p.goldInBank = 100 * kGp;
    CHECK(m::bankWithdrawGold(p, 40 * kGp));
    CHECK_EQ(p.goldInBank, 60 * kGp);
    CHECK_EQ(p.goldOnHand, 90 * kGp);
}

// Shelving: floor 0 with Unaligned is not shelved; any other floor is shelved.
TEST(loot2_shelved_by_selling_floor_zero_unaligned) {
    CHECK(!m::shelvedBySelling(0, m::ItemAlign::Unaligned));
}

TEST(loot2_shelved_by_selling_other_floors) {
    CHECK(m::shelvedBySelling(1, m::ItemAlign::Unaligned));
    CHECK(m::shelvedBySelling(2, m::ItemAlign::Evil));
}

// Slot curse is a boolean: any nonzero curse kind sets it.
TEST(loot2_slot_cursed_any_curse_kind) {
    m::Item item;
    CHECK(!m::slotCursedFromItem(item));
    item.cursed = m::CurseKind1;
    CHECK(m::slotCursedFromItem(item));
    item.cursed = m::CurseKind2;
    CHECK(m::slotCursedFromItem(item));
}
