#include <cmath>

#include "test_support.h"
#include "mordor/loot1b.h"

namespace {

// Currency is stored in 1/10000 gold units (foundation note).
constexpr mordor::Currency kLootGold = 10000;

bool lootNear(double a, double b, double tol) {
    return std::fabs(a - b) <= tol;
}

}  // namespace

// NOT FIXED in this file: every "tsup::Env env({<double>})" below narrows
// double -> mordor::u64 because tsup::Env's constructor takes u64 elements.
// The fix belongs in test_support.h: Env must accept the double sequence
// (Rnd values in [0,1)). Changing the literals here would change test inputs,
// and test_support.h was not provided, so this file still fails to compile.

// T2: per-class modifiers.
TEST(Loot1b_ClassModifierTable) {
    using mordor::ChestClass;
    CHECK(mordor::chestGoldFactor(ChestClass::NONE) == 0.5);
    CHECK(mordor::chestGoldFactor(ChestClass::CONTAINER) == 0.5);
    CHECK(mordor::chestGoldFactor(ChestClass::BOX) == 1.0);
    CHECK(mordor::chestGoldFactor(ChestClass::CHEST) == 1.5);

    CHECK_EQ(mordor::chestItemRollAdjust(ChestClass::CONTAINER), 15);
    CHECK_EQ(mordor::chestItemRollAdjust(ChestClass::BOX), 0);
    CHECK_EQ(mordor::chestItemRollAdjust(ChestClass::CHEST), -25);

    CHECK_EQ(mordor::chestItemBandAdjust(ChestClass::CONTAINER), -1);
    CHECK_EQ(mordor::chestItemBandAdjust(ChestClass::BOX), 0);
    CHECK_EQ(mordor::chestItemBandAdjust(ChestClass::CHEST), 0);

    CHECK_EQ(mordor::chestTrapChancePercent(ChestClass::NONE), 0);
    CHECK_EQ(mordor::chestTrapChancePercent(ChestClass::CONTAINER), 25);
    CHECK_EQ(mordor::chestTrapChancePercent(ChestClass::BOX), 75);
    CHECK_EQ(mordor::chestTrapChancePercent(ChestClass::CHEST), 90);
}

// T2 + trap chance: roll is Rnd*100 compared against the class percent.
TEST(Loot1b_ChestTrapRollUsesClassPercent) {
    using mordor::ChestClass;
    {
        tsup::Env env({0.99});
        CHECK(!mordor::rollChestTrapped(env.ctx, ChestClass::NONE));
    }
    {
        tsup::Env env({0.1});  // 10 < 25
        CHECK(mordor::rollChestTrapped(env.ctx, ChestClass::CONTAINER));
    }
    {
        tsup::Env env({0.5});  // 50 is not < 25
        CHECK(!mordor::rollChestTrapped(env.ctx, ChestClass::CONTAINER));
    }
    {
        tsup::Env env({0.5});  // 50 < 90
        CHECK(mordor::rollChestTrapped(env.ctx, ChestClass::CHEST));
    }
    {
        tsup::Env env({0.95});  // 95 is not < 90
        CHECK(!mordor::rollChestTrapped(env.ctx, ChestClass::CHEST));
    }
}

// T1 bonus: 25 only for a lair monster with BoxChance[2] > 0.
TEST(Loot1b_TreasureClassBonusRule) {
    CHECK_EQ(mordor::treasureClassBonus(true, 10), 25);
    CHECK_EQ(mordor::treasureClassBonus(true, 0), 0);
    CHECK_EQ(mordor::treasureClassBonus(false, 10), 0);
    CHECK_EQ(mordor::treasureClassBonus(false, 0), 0);
}

// T4: item band low end by drop level.
TEST(Loot1b_ItemBandLowEndWorkedValues) {
    CHECK_EQ(mordor::itemBandLowEnd(1), 1);
    CHECK_EQ(mordor::itemBandLowEnd(2), 1);
    CHECK_EQ(mordor::itemBandLowEnd(3), 2);
    CHECK_EQ(mordor::itemBandLowEnd(7), 4);
    CHECK_EQ(mordor::itemBandLowEnd(10), 5);
    CHECK_EQ(mordor::itemBandLowEnd(16), 8);
}

// T4 table constant must agree with the function for levels 1..16.
TEST(Loot1b_ItemBandLowEndMatchesTableConstant) {
    for (mordor::u32 i = 0; i < 16; ++i) {
        mordor::i32 level = static_cast<mordor::i32>(i) + 1;
        CHECK_EQ(mordor::itemBandLowEnd(level), mordor::kItemBandLowEnd[i]);
    }
}

// Q1: magic lock raises the roll by Log(level+10)*5 (natural log).
TEST(Loot1b_MagicLockRollIncrease) {
    CHECK(lootNear(mordor::magicLockRollIncrease(0), 5.0 * std::log(10.0), 1e-9));
    CHECK(lootNear(mordor::magicLockRollIncrease(5), 5.0 * std::log(15.0), 1e-9));
}

// T5 / Q5: the gold depth term is reproduced as written, not as intended.
TEST(Loot1b_GoldDepthTermAsWritten) {
    CHECK(lootNear(mordor::goldDepthTermAsWritten(1), 1.0597, 0.0005));
    CHECK(lootNear(mordor::goldDepthTermAsWritten(5), 2.1584, 0.0005));
    CHECK(lootNear(mordor::goldDepthTermAsWritten(15), 3.1391, 0.0005));
    // Intended Log(6)/Log(2) is about 2.585; the written term must differ.
    CHECK(!lootNear(mordor::goldDepthTermAsWritten(5), 2.585, 0.05));
}

// T6: opening rounds by disarm; 60 and above floor to 2.
TEST(Loot1b_OpeningRoundsByDisarm) {
    CHECK_EQ(mordor::openingRoundsForDisarm(12), 9);
    CHECK_EQ(mordor::openingRoundsForDisarm(20), 5);
    CHECK_EQ(mordor::openingRoundsForDisarm(60), 2);
    CHECK_EQ(mordor::openingRoundsForDisarm(99), 2);
}

// Q27: disarm display is capped at 98.
TEST(Loot1b_CapDisarmForDisplayAt98) {
    CHECK_EQ(mordor::capDisarmForDisplay(50), 50);
    CHECK_EQ(mordor::capDisarmForDisplay(97), 97);
    CHECK_EQ(mordor::capDisarmForDisplay(98), 98);
    CHECK_EQ(mordor::capDisarmForDisplay(99), 98);
    CHECK_EQ(mordor::capDisarmForDisplay(150), 98);
}

// T3: TrapFlags bit index equals the ChestTrap index.
TEST(Loot1b_TrapFlagBitIndexing) {
    using mordor::ChestTrap;
    // bits 2 (Disease Spores) and 7 (Teleporter, Trap7) set: 4 + 128
    const mordor::i32 flags = 132;
    CHECK(mordor::chestTrapFlagSet(flags, ChestTrap::DiseaseSpores));
    CHECK(mordor::chestTrapFlagSet(flags, ChestTrap::Trap7));
    CHECK(!mordor::chestTrapFlagSet(flags, ChestTrap::PoisonGas));
    CHECK(!mordor::chestTrapFlagSet(flags, ChestTrap::Fire));
}

// Q12 / T3: fire damage is reduced by Fire resistance/2 percent.
TEST(Loot1b_FireTrapResistHalvesPercent) {
    CHECK_EQ(mordor::fireTrapDamageAfterResist(20, 0), 20);
    CHECK_EQ(mordor::fireTrapDamageAfterResist(20, 50), 15);   // 25% off
    CHECK_EQ(mordor::fireTrapDamageAfterResist(20, 100), 10);  // 50% off
}

// Q11: poison test as written: caught when Rnd*100 < resistance/2.
TEST(Loot1b_PoisonGasPolarityAsWritten) {
    {
        tsup::Env env({0.0});  // zero resistance: 0 < 0 is false, immune
        CHECK(!mordor::poisonGasCatches(env.ctx, 0));
    }
    {
        tsup::Env env({0.99});
        CHECK(!mordor::poisonGasCatches(env.ctx, 0));
    }
    {
        tsup::Env env({0.1});  // resistance 50 -> threshold 25; 10 < 25
        CHECK(mordor::poisonGasCatches(env.ctx, 50));
    }
    {
        tsup::Env env({0.5});  // 50 is not < 25
        CHECK(!mordor::poisonGasCatches(env.ctx, 50));
    }
}

// T8: paid shares use CLng (round half to even), in whole gold.
TEST(Loot1b_SplitSharePaidRoundsHalfEven) {
    CHECK_EQ(mordor::splitSharePaid(5 * kLootGold, 2), 2 * kLootGold);   // 2.5 -> 2
    CHECK_EQ(mordor::splitSharePaid(7 * kLootGold, 2), 4 * kLootGold);   // 3.5 -> 4
    CHECK_EQ(mordor::splitSharePaid(10 * kLootGold, 3), 3 * kLootGold);  // 3.33 -> 3
    CHECK_EQ(mordor::splitSharePaid(10 * kLootGold, 4), 2 * kLootGold);  // 2.5 -> 2
    CHECK_EQ(mordor::splitSharePaid(14 * kLootGold, 4), 4 * kLootGold);  // 3.5 -> 4
}

// Q8: the quoted share and the paid share differ at a half.
TEST(Loot1b_SplitShareQuotedMismatchAtHalf) {
    const mordor::Currency quoted = mordor::splitShareQuoted(5 * kLootGold, 2);
    const mordor::Currency paid = mordor::splitSharePaid(5 * kLootGold, 2);
    CHECK_EQ(quoted, 3 * kLootGold);  // shown as "3"
    CHECK_EQ(paid, 2 * kLootGold);    // paid 2 each
    CHECK(quoted != paid);
}

// T7: 10.5 displays as "11" (half rounds up in display).
TEST(Loot1b_FormatWholeGoldHalfUp) {
    const mordor::Currency tenAndHalf = 10 * kLootGold + kLootGold / 2;
    CHECK_EQ(mordor::formatWholeGold(tenAndHalf), mordor::Str("11"));
}

// T7 / Q32: thief take uses CLng of the purse when the roll exceeds it.
TEST(Loot1b_ThiefTakeRoundsPurse) {
    {
        tsup::Env env({0.5});
        mordor::Purse p;
        p.goldOnHand = 10 * kLootGold + kLootGold / 2;  // 10.5
        mordor::Currency took = mordor::thiefTakeGold(env.ctx, p, 20 * kLootGold);
        CHECK_EQ(took, 10 * kLootGold);            // 10.5 -> 10
        CHECK_EQ(p.goldOnHand, kLootGold / 2);     // 0.5 stays
    }
    {
        tsup::Env env({0.5});
        mordor::Purse p;
        p.goldOnHand = 11 * kLootGold + kLootGold / 2;  // 11.5
        mordor::Currency took = mordor::thiefTakeGold(env.ctx, p, 20 * kLootGold);
        CHECK_EQ(took, 12 * kLootGold);  // 11.5 -> 12
        CHECK_EQ(p.goldOnHand, 0);       // -0.5 zeroed
    }
}

// Q33: give takes CLng of the amount; the fraction is not moved.
TEST(Loot1b_GiveGoldWholeAmountTransfer) {
    tsup::Env env({0.5});
    mordor::Purse giver;
    mordor::Purse receiver;
    giver.goldOnHand = 10 * kLootGold;
    mordor::giveGold(env.ctx, giver, receiver, 2 * kLootGold + kLootGold / 2);  // 2.5
    CHECK_EQ(receiver.goldOnHand, 2 * kLootGold);
    CHECK_EQ(giver.goldOnHand, 8 * kLootGold);
}

// T9 / Q34: exchange in town draws hand first, shortfall comes from bank.
TEST(Loot1b_ExchangeShortfallFromBank) {
    tsup::Env env({0.5});
    mordor::Purse wallet;
    wallet.goldOnHand = 10 * kLootGold;
    wallet.goldInBank = 5 * kLootGold;
    mordor::exchangeGoldInTown(env.ctx, wallet, 12 * kLootGold);
    CHECK_EQ(wallet.goldOnHand, 0);
    CHECK_EQ(wallet.goldInBank, 3 * kLootGold);
}

// T9: unchecked withdrawal with a negative amount drives hand negative.
TEST(Loot1b_WithdrawUncheckedNegativeAmount) {
    tsup::Env env({0.5});
    mordor::Purse wallet;
    wallet.goldOnHand = kLootGold / 4;  // 0.25
    mordor::withdrawGoldUnchecked(env.ctx, wallet, -5 * kLootGold);
    CHECK_EQ(wallet.goldOnHand, kLootGold / 4 - 5 * kLootGold);  // -4.75
}

// Q4: a generation pass clears the trap but never the lock.
TEST(Loot1b_GenerationPassClearsTrapKeepsLock) {
    tsup::Env env({0.5});
    mordor::SquareState chest;
    chest.hasChest = true;
    chest.chestTrap = mordor::ChestTrap::Fire;
    chest.chestLock = mordor::ChestLock::MAGIC_LOCKED;
    mordor::beginChestGenerationPass(env.ctx, chest);
    CHECK(chest.chestTrap == mordor::ChestTrap::NotTrapped);
    CHECK(chest.chestLock == mordor::ChestLock::MAGIC_LOCKED);
}

// Q47 / T10: only curse kind 2 attaches itself.
TEST(Loot1b_CurseAttachesOnlyKindTwo) {
    mordor::Item it;
    it.cursed = mordor::CurseNone;
    CHECK(!mordor::curseAttachesItself(it));
    it.cursed = mordor::CurseKind1;  // Bone Dagger type, stays in pack
    CHECK(!mordor::curseAttachesItself(it));
    it.cursed = mordor::CurseKind2;
    CHECK(mordor::curseAttachesItself(it));
}

// Q6: group gold under 500 gold is replaced by Int(Rnd*500)+2 gold.
TEST(Loot1b_ChestGoldFloorUnder500) {
    {
        tsup::Env env({0.0});
        CHECK_EQ(mordor::applyChestGoldFloor(env.ctx, 100 * kLootGold), 2 * kLootGold);
    }
    {
        tsup::Env env({0.5});  // Int(250) + 2 = 252
        CHECK_EQ(mordor::applyChestGoldFloor(env.ctx, 100 * kLootGold), 252 * kLootGold);
    }
    {
        tsup::Env env({0.0});  // 500 is not under 500: unchanged
        CHECK_EQ(mordor::applyChestGoldFloor(env.ctx, 500 * kLootGold), 500 * kLootGold);
    }
    {
        tsup::Env env({0.0});
        CHECK_EQ(mordor::applyChestGoldFloor(env.ctx, 600 * kLootGold), 600 * kLootGold);
    }
}

// Q7 / Q41 / Q47: payout and experience gates.
TEST(Loot1b_PayoutAndDisarmGates) {
    // Q7: an unopened chest pays no gold; an opened one pays.
    CHECK(mordor::chestGoldPayable(true, false));
    CHECK(!mordor::chestGoldPayable(false, false));
    // Q47: victory pays fight gold only when no chest is present.
    CHECK(mordor::fightGoldPaidAtVictory(false));
    CHECK(!mordor::fightGoldPaidAtVictory(true));
    // Q41: disarm awards XP only on a trapped chest.
    CHECK(mordor::disarmAwardsXp(true));
    CHECK(!mordor::disarmAwardsXp(false));
}

// Lock and key gates.
TEST(Loot1b_LockAndOpenKeyGates) {
    mordor::SquareState chest;
    chest.chestLock = mordor::ChestLock::MAGIC_LOCKED;
    CHECK(mordor::isMagicLockedChest(chest));
    CHECK(mordor::chestLockOf(chest) == mordor::ChestLock::MAGIC_LOCKED);

    chest.chestLock = mordor::ChestLock::UNLOCKED;
    CHECK(!mordor::isMagicLockedChest(chest));
    CHECK(mordor::chestLockOf(chest) == mordor::ChestLock::UNLOCKED);

    chest.chestLock = mordor::ChestLock::CHARM_OPENED;
    CHECK(!mordor::isMagicLockedChest(chest));

    // Q43: a hidden trap label blocks the Open key.
    CHECK(!mordor::openKeyEnabled(false));
    CHECK(mordor::openKeyEnabled(true));
}

// Q43: a DEX of zero on either side leaves the panel as it was.
TEST(Loot1b_TrapPanelZeroDexKeepsPriorState) {
    mordor::Character viewer;  // DEX defaults to 0
    {
        tsup::Env env({0.5});
        CHECK(mordor::chestTrapPanelShown(env.ctx, viewer, 12, true));
    }
    {
        tsup::Env env({0.5});
        CHECK(!mordor::chestTrapPanelShown(env.ctx, viewer, 12, false));
    }
    viewer.abilities.stats[5] = 15;  // DEX set, monster DEX zero
    {
        tsup::Env env({0.5});
        CHECK(mordor::chestTrapPanelShown(env.ctx, viewer, 0, true));
    }
    {
        tsup::Env env({0.5});
        CHECK(!mordor::chestTrapPanelShown(env.ctx, viewer, 0, false));
    }
}
