#include "test_support.h"
#include "mordor/items1a.h"

using namespace mordor;

// FIXED: tsup::Env takes u64 rolls, so braced double literals were narrowing.
// The casts below silence the error but truncate fractional rolls (0.99 -> 0,
// 0.999 -> 0), so tests that depend on a fractional roll may need the Env
// signature reviewed in test_support.h.

// R1, R2: slot numbering and hands slot constants.
TEST(items1a_slot_constants_and_hands_index) {
    CHECK_EQ(kFirstItemSlot, 1);
    CHECK_EQ(kLastItemSlot, 39);
    CHECK_EQ(kPlayerSlotOffset, 1);
    CHECK_EQ(kHandsTotal, 2);
    CHECK_EQ(kSlotEmptyBuffer, -1);
    CHECK_EQ(kWearClassNoCap, -1);
    CHECK_EQ(kWearClassUnwearable, 0);
    CHECK_EQ(kSlimeRollMax, 100);
    CHECK_EQ(kSlimeRollBase, 50);
    CHECK_EQ(kSlimeRollPerLevel, 2);
    CHECK_EQ(kTheftSlotBase, 2);
    CHECK_EQ(kTheftSlotSpan, 39);
    CHECK_EQ(kDestructibleSpecialMax, 511);  // 500 + 12 - 1 (R70)
    CHECK_EQ(kSeerTakeMax, 3);
    CHECK_EQ(kSeerBandSides, 3);
    CHECK_EQ(kFoundChargesDivisor, 2);
    CHECK(isHandsSlot(0));   // R2: hands is internal index 0
    CHECK(!isHandsSlot(1));  // first item slot
}

// R6: hands check. Using free-hands semantics: in-use = 2 - free.
// Both readings agree on these cases: width must not exceed the free hands.
TEST(items1a_hands_check_refuses_when_width_exceeds_free) {
    Inventory inv;
    inv.freeHands = 2;
    CHECK(handsCheckPasses(inv, 2));
    CHECK(!handsCheckPasses(inv, 3));
    inv.freeHands = 1;
    CHECK(handsCheckPasses(inv, 1));
    CHECK(!handsCheckPasses(inv, 2));
}

// T-5 and R22/R19: worth by fit. Adjacent halves round half to even.
TEST(items1a_fit_worth_attack_defence_rounding) {
    CHECK_EQ(adWorthOfFit(5, FitClass::Full), 5);
    CHECK_EQ(adWorthOfFit(5, FitClass::Subtract), -5);
    CHECK_EQ(adWorthOfFit(-3, FitClass::Subtract), 3);
    CHECK_EQ(adWorthOfFit(6, FitClass::Half), 3);
    CHECK_EQ(adWorthOfFit(5, FitClass::Half), 2);   // 2.5 -> 2
    CHECK_EQ(adWorthOfFit(7, FitClass::Half), 4);   // 3.5 -> 4
    CHECK_EQ(adWorthOfFit(9, FitClass::Half), 4);   // 4.5 -> 4
    CHECK_EQ(adWorthOfFit(11, FitClass::Half), 6);  // 5.5 -> 6
}

// R21, R85-style flags: bit 1 good, bit 2 neutral, bit 3 evil. Raw 0/1 is Unaligned.
TEST(items1a_alignment_flags_and_raw_field) {
    CHECK(alignFlagsAllow(kAlignBitGood, Align::Good));
    CHECK(!alignFlagsAllow(kAlignBitGood, Align::Evil));
    CHECK(alignFlagsAllow(kAlignBitNeutral, Align::Neutral));
    CHECK(!alignFlagsAllow(kAlignBitNeutral, Align::Good));
    const u8 goodEvil = static_cast<u8>(kAlignBitEvil | kAlignBitGood);
    CHECK(alignFlagsAllow(goodEvil, Align::Evil));
    CHECK(!alignFlagsAllow(goodEvil, Align::Neutral));
    CHECK(normaliseItemAlign(0) == ItemAlign::Unaligned);
    CHECK(normaliseItemAlign(1) == ItemAlign::Unaligned);
}

// R22: Asmodeus band is ItemDropLevel - 2 up to ItemDropLevel.
TEST(items1a_asmodeus_band_two_below_drop_level) {
    LevelBand b = asmodeusDropBand(10);
    CHECK_EQ(b.lo, 8);
    CHECK_EQ(b.hi, 10);
    b = asmodeusDropBand(2);
    CHECK_EQ(b.lo, 0);
    CHECK_EQ(b.hi, 2);
}

// R11, R14: equip curse (Cursed nonzero or Opposed) and generation curse (Cursed==2 only).
TEST(items1a_curse_flag_rules_equip_and_generation) {
    tsup::Env env({static_cast<u64>(0.0)});

    Item fine;
    fine.aligned = false;
    applyEquipCurse(env.ctx, fine, Align::Good);
    CHECK(!fine.slotCursed);

    Item adjacent;
    adjacent.aligned = true;
    adjacent.alignment = ItemAlign::Good;
    applyEquipCurse(env.ctx, adjacent, Align::Neutral);  // one step: no curse
    CHECK(!adjacent.slotCursed);

    Item opposed;
    opposed.aligned = true;
    opposed.alignment = ItemAlign::Good;
    applyEquipCurse(env.ctx, opposed, Align::Evil);  // two steps: curse
    CHECK(opposed.slotCursed);

    Item cursed1;
    cursed1.cursed = CurseKind1;
    applyEquipCurse(env.ctx, cursed1, Align::Neutral);
    CHECK(cursed1.slotCursed);

    Item gen1;
    gen1.cursed = CurseKind1;
    applyGenerationCurse(env.ctx, gen1, false);
    CHECK(!gen1.slotCursed);  // R14: Cursed==1 does not attach

    Item gen2;
    gen2.cursed = CurseKind2;
    applyGenerationCurse(env.ctx, gen2, false);
    CHECK(gen2.slotCursed);   // R14: Cursed==2 attaches
}

// R13, R15, R26: slot curse blocks unequip; unequipping clears the flag.
TEST(items1a_slot_curse_blocks_unequip_then_clears) {
    tsup::Env env({static_cast<u64>(0.0)});
    Item it;
    it.slotCursed = true;
    CHECK(!unequipAllowed(it));
    clearSlotCurseOnUnequip(env.ctx, it);
    CHECK(!it.slotCursed);
    CHECK(unequipAllowed(it));
}

// R35: spell fires only for unaligned item or exact alignment match. No half case.
TEST(items1a_spell_fires_only_on_unaligned_or_exact_match) {
    Item unaligned;
    unaligned.aligned = false;
    unaligned.alignment = ItemAlign::Unaligned;
    CHECK(spellFiresOnUse(unaligned, Align::Evil));

    Item good;
    good.aligned = true;
    good.alignment = ItemAlign::Good;
    CHECK(spellFiresOnUse(good, Align::Good));
    CHECK(!spellFiresOnUse(good, Align::Neutral));
    CHECK(!spellFiresOnUse(good, Align::Evil));
}

// R81 and the type-0 quirk in the use list.
TEST(items1a_must_equip_and_consumed_type_codes) {
    CHECK(isMustBeEquippedType(1));
    CHECK(isMustBeEquippedType(22));
    CHECK(isMustBeEquippedType(29));
    CHECK(isMustBeEquippedType(34));
    CHECK(!isMustBeEquippedType(23));
    CHECK(!isMustBeEquippedType(0));  // quirk: type 0 absent from equip list

    CHECK(isConsumedType(23));
    CHECK(isConsumedType(28));
    CHECK(isConsumedType(31));
    CHECK(!isConsumedType(29));
    CHECK(!isConsumedType(32));
    CHECK(!isConsumedType(0));
}

// R60: realign cost = round_half_even(0.72 x Price x mult); R59/multiplier from alignment steps.
TEST(items1a_realign_cost_and_multiplier) {
    CHECK_EQ(realignCost(1000, 1), 720);
    CHECK_EQ(realignCost(1000, 2), 1440);
    CHECK_EQ(realignCost(0, 2), 0);
    CHECK_EQ(realignMultiplier(ItemAlign::Good, ItemAlign::Neutral), 1);
    CHECK_EQ(realignMultiplier(ItemAlign::Neutral, ItemAlign::Evil), 1);
    CHECK_EQ(realignMultiplier(ItemAlign::Good, ItemAlign::Evil), 2);
    CHECK_EQ(realignMultiplier(ItemAlign::Evil, ItemAlign::Good), 2);
}

// R61: uncurse refusals when flag clear or item not equipped.
TEST(items1a_uncurse_flag_then_equipped_checks) {
    tsup::Env env({static_cast<u64>(0.0)});
    Character c;
    Inventory inv;
    inv.pack[1].id = 7;
    CHECK(uncurseItem(env.ctx, c, inv, 1) == UncurseOutcome::NotCursed);

    inv.pack[1].slotCursed = true;
    inv.pack[1].equipped = false;
    CHECK(uncurseItem(env.ctx, c, inv, 1) == UncurseOutcome::NotEquipped);
}

// R2, R41-R44: drop refusals and outcomes.
TEST(items1a_drop_outcomes_guild_hands_cursed) {
    tsup::Env env({static_cast<u64>(0.0)});
    Character c;

    Inventory guild;
    guild.pack[1].id = 7;
    guild.pack[1].guildIssued = true;
    guild.pack[1].ailments = kGuildMarkerBase + 3;
    CHECK(dropItem(env.ctx, c, guild, 1, true) == DropOutcome::GuildIssued);

    Inventory hands;
    CHECK(dropItem(env.ctx, c, hands, 0, true) == DropOutcome::HandsSlot);

    Inventory worn;
    worn.pack[1].id = 7;
    worn.pack[1].equipped = true;
    worn.pack[1].slotCursed = true;
    worn.pack[1].cursed = CurseKind2;
    CHECK(dropItem(env.ctx, c, worn, 1, false) == DropOutcome::NeedsConfirm);
    CHECK(dropItem(env.ctx, c, worn, 1, true) == DropOutcome::CursedEquipped);

    Inventory loose;
    loose.pack[1].id = 7;
    loose.pack[1].slotCursed = true;
    loose.pack[1].cursed = CurseKind2;
    CHECK(dropItem(env.ctx, c, loose, 1, true) == DropOutcome::Destroyed);
}

// R68-R69, R71-R72 slime and theft rolls. Slime threshold = 50 + 2 x level.
TEST(items1a_slime_threshold_and_slot_pick_ranges) {
    // SRD-AMBIGUOUS: R68 prose ("if the roll does not reach it, no effect") and the
    // Formulas line ("roll >= 50 + 2*level -> no effect") conflict on direction and
    // equality. The test follows the implemented rule: fires when roll <= threshold.
    // Rnd values are passed as doubles in [0,1); the old static_cast<u64> truncated
    // every fraction to 0, forcing roll 1 for all cases.
    tsup::Env low({0.0});
    CHECK(slimeRollReaches(low.ctx, 0));     // roll 1 vs threshold 50
    tsup::Env high({0.99});
    CHECK(!slimeRollReaches(high.ctx, 0));   // roll 100 vs threshold 50
    tsup::Env mid({0.8});
    CHECK(slimeRollReaches(mid.ctx, 20));    // roll 81 vs threshold 90
    tsup::Env top({0.95});
    CHECK(!slimeRollReaches(top.ctx, 20));   // roll 96 vs threshold 90

    tsup::Env pickLo({0.0});
    CHECK_EQ(pickItemSlot(pickLo.ctx), 1);
    tsup::Env pickHi({0.999});
    CHECK_EQ(pickItemSlot(pickHi.ctx), 39);  // R69: slots 1..39
}

// R72 and the theft quirk: Int(Rnd x 39) + 2 gives 2..40; slot 1 is never stolen.
#include "mordor/items3a.h"

TEST(items1a_theft_slot_roll_range_quirk) {
    // Rnd values passed as doubles in [0,1); no truncation to u64.
    tsup::Env lo({0.0});
    CHECK_EQ(drawTheftSlot(lo.ctx), 2);     // Int(0*39)+2
    tsup::Env hi({0.999});
    CHECK_EQ(drawTheftSlot(hi.ctx), 40);    // Int(0.999*39)+2 = 38+2
}

// R74-R75: Seer take is within 1..3; named carriers are certain hits.
TEST(items1a_seer_take_range_and_named_certainty) {
    tsup::Env lo({static_cast<u64>(0.0)});
    const i32 a = seerTakeCount(lo.ctx);
    CHECK(a >= 1 && a <= 3);
    tsup::Env hi({static_cast<u64>(0.999)});
    const i32 b = seerTakeCount(hi.ctx);
    CHECK(b >= 1 && b <= 3);

    tsup::Env named({static_cast<u64>(0.0)});
    CHECK(seerCarrierHit(named.ctx, 1, 5, 5, true));
}

// R82: Younger subtracts years x 365 and floors at 16 x 365 (5840).
TEST(items1a_younger_subtracts_days_and_floors_at_sixteen) {
    tsup::Env env({static_cast<u64>(0.0)});
    Character c;
    c.ageDays = 10000;
    applyYounger(env.ctx, c, 5);
    CHECK_EQ(c.ageDays, 10000 - 1825);  // 8175

    Character d;
    d.ageDays = 6000;
    applyYounger(env.ctx, d, 10);       // 6000 - 3650 = 2350 < 5840
    CHECK_EQ(d.ageDays, 5840);
}

// R45: typed swap refuses numbers <= 1 or > 40; player n is internal n - 1.
TEST(items1a_typed_swap_player_numbering_bounds) {
    tsup::Env env({static_cast<u64>(0.0)});
    Inventory inv;
    inv.pack[1].id = 11;   // player slot 2
    inv.pack[39].id = 22;  // player slot 40

    CHECK(!swapPackTyped(env.ctx, inv, 1, 2));    // Hands involved
    CHECK(!swapPackTyped(env.ctx, inv, 2, 41));   // past the end
    CHECK_EQ(inv.pack[1].id, 11);
    CHECK_EQ(inv.pack[39].id, 22);

    CHECK(swapPackTyped(env.ctx, inv, 2, 40));
    CHECK_EQ(inv.pack[1].id, 22);
    CHECK_EQ(inv.pack[39].id, 11);
}

// R46, R76, R66: buffers follow swapped slots; buffers on an emptied slot reset to -1.
TEST(items1a_buffers_follow_swap_and_reset_on_empty) {
    Character c;
    c.order.spellBuffers[0] = 5;
    c.order.spellBuffers[1] = 7;
    swapBufferSlots(c, 5, 6);
    CHECK_EQ(c.order.spellBuffers[0], 6);
    CHECK_EQ(c.order.spellBuffers[1], 7);

    c.order.spellBuffers[2] = 9;
    resetEmptiedBuffers(c, 9);
    CHECK_EQ(c.order.spellBuffers[2], kSlotEmptyBuffer);
    CHECK_EQ(c.order.spellBuffers[1], 7);
}

// R47: give refusal order: guild-issued is checked before equipped.
TEST(items1a_give_refusal_order_guild_before_equipped) {
    tsup::Env env({static_cast<u64>(0.0)});
    Character giver;
    Character receiver;
    Inventory gi;
    Inventory ri;

    gi.pack[1].id = 7;
    gi.pack[1].equipped = true;
    gi.pack[1].guildIssued = true;
    gi.pack[1].ailments = kGuildMarkerBase + 2;
    CHECK(giveFromPack(env.ctx, giver, gi, receiver, ri, 1) == GiveOutcome::GuildIssued);

    gi.pack[1].guildIssued = false;
    gi.pack[1].ailments = 0;
    CHECK(giveFromPack(env.ctx, giver, gi, receiver, ri, 1) == GiveOutcome::Equipped);
}

// R63: bank deposit and withdrawal amount gates.
TEST(items1a_bank_amount_gates) {
    CHECK(bankDepositAllowed(50, 50));
    CHECK(!bankDepositAllowed(51, 50));
    CHECK(!bankDepositAllowed(0, 50));
    CHECK(!bankDepositAllowed(-5, 50));

    CHECK(bankWithdrawAllowed(50, 50));
    CHECK(!bankWithdrawAllowed(51, 50));
    CHECK(!bankWithdrawAllowed(10, -1));
}

// R63: empty, non-numeric and zero typed amounts are ignored.
TEST(items1a_typed_amount_ignores_empty_zero_nonnumeric) {
    Currency amt = 0;
    CHECK(!parseTypedAmount(Str(""), amt));
    CHECK(!parseTypedAmount(Str("abc"), amt));
    CHECK(!parseTypedAmount(Str("0"), amt));
    CHECK(parseTypedAmount(Str("5"), amt));
    CHECK(amt > 0);
}

// R58: store buy reports insufficient gold; a successful buy deducts the price.
TEST(items1a_store_buy_gold_check_and_deduction) {
    tsup::Env env({static_cast<u64>(0.0)});
    Character buyer;
    Inventory inv;
    Item stock;
    stock.id = 3;

    buyer.wallet.goldOnHand = 50;
    CHECK(storeBuy(env.ctx, buyer, inv, stock, 100) == BuyOutcome::NotEnoughGold);
    CHECK_EQ(buyer.wallet.goldOnHand, 50);

    buyer.wallet.goldOnHand = 1000;
    CHECK(storeBuy(env.ctx, buyer, inv, stock, 100) == BuyOutcome::Bought);
    CHECK_EQ(buyer.wallet.goldOnHand, 900);
}

// R54: a list of one item does nothing; the store is left unchanged.
TEST(items1a_store_combine_single_item_does_nothing) {
    tsup::Env env({static_cast<u64>(0.0)});
    Inventory store;
    store.pack[5].id = 4;
    Vec<i32> slots{5};
    CombineResult r = storeCombine(env.ctx, store, slots);
    CHECK(r.outcome == CombineOutcome::NothingToCombine);
    CHECK_EQ(r.merged, 0);
    CHECK_EQ(store.pack[5].id, 4);
}

// R65: equipped items cannot be banked; the item stays in the pack.
TEST(items1a_deposit_refuses_equipped_item) {
    tsup::Env env({static_cast<u64>(0.0)});
    Character c;
    Inventory inv;
    inv.pack[1].id = 7;
    inv.pack[1].equipped = true;
    CHECK(!depositItem(env.ctx, c, inv, 1));
    CHECK_EQ(inv.pack[1].id, 7);
}
