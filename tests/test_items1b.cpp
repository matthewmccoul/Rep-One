#include "test_support.h"
#include "mordor/items1b.h"
#include "mordor/items1a.h"  // FIXED: identifyChance(f64, bool) is declared here

using namespace mordor;

namespace {

Item& packAt(Inventory& inv, i32 i) { return inv.pack[static_cast<u32>(i)]; }
const Item& packAt(const Inventory& inv, i32 i) { return inv.pack[static_cast<u32>(i)]; }

void fillPack(Inventory& inv) {
    for (i32 i = 1; i < kPackSlots; ++i) packAt(inv, i).id = 1;
}

}  // namespace

// Destroy chance: (45 - LevelFound) + (dex+dex)*2 - monDex*2, paralysed uses
// (dex+dex)/2.0 in place of the dex term; clamped to 25..90.
TEST(destroy_chance_formula_paralysis_and_clamp) {
    CHECK_EQ(destroyChance(10, 10, 5, false), 65);   // 35 + 40 - 10
    CHECK_EQ(destroyChance(10, 11, 5, false), 69);   // 35 + 44 - 10
    CHECK_EQ(destroyChance(45, 0, 0, false), 25);    // 0 clamped up
    CHECK_EQ(destroyChance(0, 40, 0, false), 90);    // 45 + 160 clamped down
    CHECK_EQ(destroyChance(10, 10, 5, true), 35);    // 35 + 10 - 10
    CHECK_EQ(destroyChance(10, 11, 5, true), 36);    // 35 + 11 - 10
    CHECK_EQ(destroyChance(45, 0, 0, true), 25);
}

// Guild crests occupy 500..511 (500 + 12 - 1); they are protected.
TEST(destructible_excludes_guild_crests) {
    CHECK(isDestructible(1));
    CHECK(isDestructible(13));
    CHECK(isDestructible(499));
    CHECK(!isDestructible(500));
    CHECK(!isDestructible(511));
    CHECK(isDestructible(512));
}

// Strike when Int(Rnd*100)+1 < 50 + 2*dungeonLevel.
TEST(slime_strike_threshold_by_dungeon_level) {
    tsup::Env a({0.485, 0.485});   // Int(48.5)=48 -> roll 49 < 50
    CHECK(slimeStrikes(a.ctx, 0));
    tsup::Env b({0.495, 0.495});   // Int(49.5)=49 -> roll 50, not < 50
    CHECK(!slimeStrikes(b.ctx, 0));
    tsup::Env c({0.685, 0.685});   // roll 69 < 70 at level 10
    CHECK(slimeStrikes(c.ctx, 10));
    tsup::Env d({0.695, 0.695});   // roll 70, not < 70
    CHECK(!slimeStrikes(d.ctx, 10));
}

// Slot draw is Int(Rnd*39)+1 for pack; theft slot is Int(Rnd*39)+2.
TEST(slime_slot_and_theft_slot_ranges) {
    tsup::Env a({0.0, 0.0});
    CHECK_EQ(slimeSlot(a.ctx), 1);
    tsup::Env b({0.5, 0.5});       // Int(19.5)=19
    CHECK_EQ(slimeSlot(b.ctx), 20);
    tsup::Env c({0.999, 0.999});   // Int(38.961)=38
    CHECK_EQ(slimeSlot(c.ctx), 39);

    tsup::Env d({0.0, 0.0});
    CHECK_EQ(theftSlot(d.ctx), 2);
    tsup::Env e({0.5, 0.5});
    CHECK_EQ(theftSlot(e.ctx), 21);
    tsup::Env f({0.999, 0.999});
    CHECK_EQ(theftSlot(f.ctx), 40);
}

// v = score - 2*monLvl; gold if v < 50, item if v <= 10.
TEST(theft_flag_thresholds) {
    TheftFlags a = theftFlags(60, 5);   // v = 50
    CHECK(!a.gold);
    CHECK(!a.item);
    TheftFlags b = theftFlags(59, 5);   // v = 49
    CHECK(b.gold);
    CHECK(!b.item);
    TheftFlags c = theftFlags(20, 5);   // v = 10
    CHECK(c.gold);
    CHECK(c.item);
    TheftFlags d = theftFlags(21, 5);   // v = 11
    CHECK(d.gold);
    CHECK(!d.item);
}

// gold = Int(Rnd * ((DL + monAtt) * (3353.452 * monLvl)) / max(1, GL))
TEST(slime_gold_formula) {
    // (2+3) * (3353.452*2) = 33534.52
    tsup::Env a({0.5, 0.5});
    // 33534.52 / 2 = 16767.26; *0.5 = 8383.63
    CHECK_EQ(slimeGold(a.ctx, 2, 3, 2, 2), 8383);
    tsup::Env b({0.5, 0.5});
    // GuildLevel 0 is treated as 1: 33534.52 * 0.5 = 16767.26
    CHECK_EQ(slimeGold(b.ctx, 2, 3, 2, 0), 16767);
}

// Halving helpers and item worth by alignment fit.
TEST(halving_helpers_and_worth_by_fit) {
    CHECK_EQ(attackDefenceHalved(5), 2);
    CHECK_EQ(attackDefenceHalved(4), 2);
    CHECK_EQ(attackDefenceHalved(6), 3);
    CHECK_EQ(statModHalved(5), 2);     // Int(2.5)
    CHECK_EQ(statModHalved(7), 3);     // Int(3.5)
    CHECK_EQ(statModHalved(6), 3);

    // AttDefWorth: Fine raw; Adjacent round_half_even(raw/2.0); Opposed -raw
    CHECK_EQ(attDefWorth(9, FitClass::Full), 9);
    CHECK_EQ(attDefWorth(7, FitClass::Half), 4);     // 3.5 -> 4 (even)
    CHECK_EQ(attDefWorth(5, FitClass::Half), 2);     // 2.5 -> 2 (even)
    CHECK_EQ(attDefWorth(9, FitClass::Subtract), -9);

    // StatModWorth: Adjacent floor(raw/2.0); Opposed -raw
    CHECK_EQ(statModWorth(7, FitClass::Full), 7);
    CHECK_EQ(statModWorth(5, FitClass::Half), 2);
    CHECK_EQ(statModWorth(-5, FitClass::Half), -3);  // floor(-2.5)
    CHECK_EQ(statModWorth(4, FitClass::Subtract), -4);
    CHECK_EQ(statModWorth(-4, FitClass::Full), -4);
}

// FitFor: unaligned or match -> Full; good vs evil -> Subtract; other -> Half.
TEST(fit_for_alignment_table) {
    CHECK(fitFor(ItemAlign::Unaligned, Align::Evil) == FitClass::Full);
    CHECK(fitFor(ItemAlign::Good, Align::Good) == FitClass::Full);
    CHECK(fitFor(ItemAlign::Evil, Align::Evil) == FitClass::Full);
    CHECK(fitFor(ItemAlign::Good, Align::Evil) == FitClass::Subtract);
    CHECK(fitFor(ItemAlign::Evil, Align::Good) == FitClass::Subtract);
    CHECK(fitFor(ItemAlign::Good, Align::Neutral) == FitClass::Half);
    CHECK(fitFor(ItemAlign::Neutral, Align::Good) == FitClass::Half);
}

// Effect bits count iff item unaligned or (item-1) == character alignment;
// hasEffect reads bit positions; mask is six bits.
TEST(effect_bits_and_alignment_gate) {
    CHECK(effectBitCounts(ItemAlign::Unaligned, Align::Evil));
    CHECK(effectBitCounts(ItemAlign::Good, Align::Good));
    CHECK(!effectBitCounts(ItemAlign::Good, Align::Neutral));
    CHECK(effectBitCounts(ItemAlign::Evil, Align::Evil));
    CHECK(effectBitCounts(ItemAlign::Neutral, Align::Neutral));
    CHECK(!effectBitCounts(ItemAlign::Neutral, Align::Good));

    CHECK(hasEffect(0x05u, EffectBit::Levitate));
    CHECK(hasEffect(0x05u, EffectBit::Protect));
    CHECK(!hasEffect(0x05u, EffectBit::Invisible));
    CHECK(hasEffect(0x20u, EffectBit::Backstab));
    CHECK_EQ(kEffectMask, 0x3Fu);
}

// Curse: CanUnequip = !SlotCursed; AttachesItself iff Cursed == 2;
// ApplyEquipCurse sets SlotCursed on any curse or an Opposed fit;
// Uncurse order: not cursed, then not equipped.
TEST(curse_flags_and_uncurse_order) {
    tsup::Env env({0.5, 0.9});

    Item it;
    CHECK(canUnequip(it));
    it.slotCursed = true;
    CHECK(!canUnequip(it));

    Item a;
    a.cursed = CurseKind2;
    CHECK(attachesItself(a));
    a.cursed = CurseKind1;
    CHECK(!attachesItself(a));

    Item c;
    c.cursed = CurseKind1;
    applyEquipCurse(env.ctx, c, FitClass::Full);
    CHECK(c.slotCursed);

    Item d;
    applyEquipCurse(env.ctx, d, FitClass::Subtract);
    CHECK(d.slotCursed);

    Item e;
    applyEquipCurse(env.ctx, e, FitClass::Half);
    CHECK(!e.slotCursed);

    Item u;
    CHECK(checkUncurse(u) == UncurseResult::NotCursed);
    u.slotCursed = true;
    CHECK(checkUncurse(u) == UncurseResult::NotEquipped);
    u.equipped = true;
    CHECK(checkUncurse(u) == UncurseResult::Ok);

    // Forced unequip never clears a cursed slot.
    Character ch;
    Inventory inv;
    Item f;
    f.equipped = true;
    f.slotCursed = true;
    forcedUnequipCheck(env.ctx, ch, inv, f);
    CHECK(f.equipped);
}

// checkSwap: out-of-range internal indices refuse; guild-issued and
// equipped/banked items are refused with their own codes.
TEST(check_swap_refusal_codes) {
    Inventory inv;
    CHECK(checkSwap(inv, 0, inv, 5) == SwapResult::OutOfRange);
    CHECK(checkSwap(inv, 5, inv, 40) == SwapResult::OutOfRange);
    CHECK(checkSwap(inv, 5, inv, 6) == SwapResult::Ok);

    packAt(inv, 5).ailments = kGuildMarkerBase;
    CHECK(checkSwap(inv, 5, inv, 6) == SwapResult::SourceGuildIssued);
    packAt(inv, 5).ailments = 0;

    packAt(inv, 6).ailments = kGuildMarkerBase;
    CHECK(checkSwap(inv, 5, inv, 6) == SwapResult::DestGuildIssued);
    packAt(inv, 6).ailments = 0;

    packAt(inv, 6).equipped = true;
    CHECK(checkSwap(inv, 5, inv, 6) == SwapResult::DestEquippedOrBanked);
}

// swapPackSlots refuses out-of-range internal indices and guild-issued
// source items; otherwise the two pack entries exchange.
TEST(swap_pack_slots_refusals_and_exchange) {
    tsup::Env env({0.5, 0.9});
    Inventory inv;
    packAt(inv, 2).id = 7;
    packAt(inv, 3).id = 9;

    swapPackSlots(env.ctx, inv, 0, inv, 3);
    CHECK_EQ(packAt(inv, 2).id, 7);
    CHECK_EQ(packAt(inv, 3).id, 9);

    swapPackSlots(env.ctx, inv, 2, inv, 40);
    CHECK_EQ(packAt(inv, 2).id, 7);
    CHECK_EQ(packAt(inv, 3).id, 9);

    swapPackSlots(env.ctx, inv, 2, inv, 3);
    CHECK_EQ(packAt(inv, 2).id, 9);
    CHECK_EQ(packAt(inv, 3).id, 7);

    packAt(inv, 2).ailments = kGuildMarkerBase;
    swapPackSlots(env.ctx, inv, 2, inv, 3);
    CHECK_EQ(packAt(inv, 2).id, 9);
    CHECK_EQ(packAt(inv, 3).id, 7);
}

// swapPlayerSlots uses player numbering: valid player slots are 2..40,
// and player N maps to pack[N-1].
TEST(swap_player_slots_uses_player_numbering) {
    tsup::Env env({0.5, 0.9});
    Character c;
    Inventory inv;
    packAt(inv, 1).id = 4;
    packAt(inv, 2).id = 5;

    swapPlayerSlots(env.ctx, c, inv, 1, 3, false);
    CHECK_EQ(packAt(inv, 1).id, 4);

    swapPlayerSlots(env.ctx, c, inv, 2, 41, false);
    CHECK_EQ(packAt(inv, 1).id, 4);

    swapPlayerSlots(env.ctx, c, inv, 2, 3, false);
    CHECK_EQ(packAt(inv, 1).id, 5);
    CHECK_EQ(packAt(inv, 2).id, 4);
}

// Slot predicates, first free slot, and spell buffer encoding.
TEST(slot_predicates_free_slot_and_buffers) {
    Inventory inv;
    CHECK(!isValidPlayerSlot(1));
    CHECK(isValidPlayerSlot(2));
    CHECK(isValidPlayerSlot(40));
    CHECK(!isValidPlayerSlot(41));
    CHECK(isHandsSlot(0));
    CHECK(!isHandsSlot(1));

    CHECK(!slotEmpty(inv, 0));
    CHECK(slotEmpty(inv, 1));
    CHECK(slotEmpty(inv, 39));
    CHECK(!slotEmpty(inv, 40));

    CHECK_EQ(firstFreeSlot(inv), 1);
    packAt(inv, 1).id = 3;
    packAt(inv, 2).id = 3;
    CHECK_EQ(firstFreeSlot(inv), 3);
    fillPack(inv);
    CHECK_EQ(firstFreeSlot(inv), -1);
    packAt(inv, 17).id = 0;
    CHECK_EQ(firstFreeSlot(inv), 17);

    CHECK_EQ(bufferForItem(7), 7);
    CHECK_EQ(bufferForSpell(0), 40);
    CHECK_EQ(bufferForSpell(9), 49);
    CHECK_EQ(kSpellBufferEmpty, -1);
}

// Use classes by numeric type: must-equip 1-22,29,32,33,34;
// consumed 23-28,30,31; combinable 23-28,30,31.
TEST(use_classes_by_type_number) {
    CHECK(!mustBeEquipped(0));
    CHECK(mustBeEquipped(1));
    CHECK(mustBeEquipped(22));
    CHECK(!mustBeEquipped(23));
    CHECK(mustBeEquipped(29));
    CHECK(mustBeEquipped(34));
    CHECK(!mustBeEquipped(30));
    CHECK(!mustBeEquipped(35));

    CHECK(!isConsumedType(22));
    CHECK(isConsumedType(23));
    CHECK(isConsumedType(28));
    CHECK(!isConsumedType(29));
    CHECK(isConsumedType(30));
    CHECK(isConsumedType(31));
    CHECK(!isConsumedType(32));

    CHECK(!isCombinableType(22));
    CHECK(isCombinableType(23));
    CHECK(isCombinableType(28));
    CHECK(!isCombinableType(29));
    CHECK(isCombinableType(30));
    CHECK(isCombinableType(31));
    CHECK(!isCombinableType(32));
}

// Identify/recognise/rename chances: formula then clamp.
TEST(identify_recognise_rename_chances_clamp) {
    // Identify chance is the skill-to-chance function identifyChance(skill, darkness) from items1a.h
    // (FIXED: was identifyChanceFromSkill, which is declared nowhere).
    // The test passes a skill value directly, so it does not need a Character.
    // Chances formula: clamp(int((dark ? skill*0.75 : skill) + 20), 10, 90). // R18/T-8
    CHECK_EQ(identifyChance(40.0, false), 60);   // 40 + 20
    CHECK_EQ(identifyChance(40.0, true), 50);    // 30 + 20
    CHECK_EQ(identifyChance(0.0, false), 20);
    CHECK_EQ(identifyChance(-100.0, false), 10); // clamp low
    CHECK_EQ(identifyChance(200.0, false), 90);  // clamp high

    CHECK_EQ(recogniseChance(40.0, false), 70);  // 50 + 20
    CHECK_EQ(recogniseChance(40.0, true), 60);   // 40 + 20
    CHECK_EQ(recogniseChance(-100.0, false), 5); // clamp low
    CHECK_EQ(recogniseChance(200.0, true), 80);  // clamp high

    CHECK_EQ(renameChance(40.0, false), 60);
    CHECK_EQ(renameChance(40.0, true), 50);      // 30 + 20
    CHECK_EQ(renameChance(-100.0, false), 10);   // clamp low
    CHECK_EQ(renameChance(200.0, false), 90);    // clamp high
}

// Found charges: coin 0 adds, coin 1 subtracts Int(Rnd*C/2); no change at C<=1.
TEST(found_charges_coin_and_draw) {
    tsup::Env a({0.9, 0.5});               // coin 1 -> -1; Int(0.5*10/2)=2
    CHECK_EQ(rollFoundCharges(a.ctx, 10), 8);

    tsup::Env b({0.1, 0.5});               // coin 0 -> +1; Int(2.5)=2
    CHECK_EQ(rollFoundCharges(b.ctx, 10), 12);

    tsup::Env c({0.1, 0.999});             // +1; Int(0.999*3/2)=1
    CHECK_EQ(rollFoundCharges(c.ctx, 3), 4);

    tsup::Env d({0.9, 0.5});
    CHECK_EQ(rollFoundCharges(d.ctx, 1), 1);
    CHECK_EQ(rollFoundCharges(d.ctx, 0), 0);
}

// Found alignment: 0 or 1 means Unaligned; a single allowed bit is forced.
TEST(found_alignment_fixed_cases) {
    tsup::Env env({0.5, 0.5});
    CHECK(rollFoundAlignment(env.ctx, 0x0F, 0) == ItemAlign::Unaligned);
    CHECK(rollFoundAlignment(env.ctx, 0x0F, 1) == ItemAlign::Unaligned);
    CHECK(rollFoundAlignment(env.ctx, 0x0F, 4) == ItemAlign::Neutral);
    CHECK(rollFoundAlignment(env.ctx, 0x0F, 8) == ItemAlign::Evil);
}

// Realign: mult 2 when |slot - (store+1)| > 1; cost = round_half_even(0.72*P*mult).
TEST(realign_cost_and_refusal) {
    CHECK_EQ(realignCost(100, 1, 0), 72);    // |1-1|=0
    CHECK_EQ(realignCost(100, 2, 1), 72);    // |2-2|=0
    CHECK_EQ(realignCost(100, 1, 2), 144);   // |1-3|=2
    CHECK_EQ(realignCost(100, 3, 0), 144);   // |3-1|=2
    CHECK_EQ(realignCost(10, 1, 2), 14);     // 14.4 -> 14

    Item it;
    CHECK(canRealign(it));
    it.equipped = true;
    CHECK(!canRealign(it));
}

// Seer: band = Int(Rnd*3); certain match on entry -(id+2) when entry < -1;
// take = round_half_even(n - Rnd*n), clamped to 1..min(3,n).
TEST(seer_band_match_and_take) {
    tsup::Env a({0.0, 0.0});
    CHECK_EQ(rollSeerBand(a.ctx), 0);
    tsup::Env b({0.34, 0.34});             // Int(1.02)=1
    CHECK_EQ(rollSeerBand(b.ctx), 1);
    tsup::Env c({0.7, 0.7});               // Int(2.1)=2
    CHECK_EQ(rollSeerBand(c.ctx), 2);

    CHECK(seerMatchesById(-7, 5));
    CHECK(!seerMatchesById(-7, 4));
    CHECK(seerMatchesById(-2, 0));
    CHECK(!seerMatchesById(-1, -1));       // entry must be < -1
    CHECK(!seerMatchesById(5, 5));

    tsup::Env d({0.0, 0.0});
    CHECK_EQ(seerTakeCount(d.ctx, 5), 3);  // clamp max 3
    tsup::Env e({0.0, 0.0});
    CHECK_EQ(seerTakeCount(e.ctx, 2), 2);  // clamp max n
    tsup::Env f({0.99, 0.99});
    CHECK_EQ(seerTakeCount(f.ctx, 2), 1);  // round(0.02)=0, clamp min 1
    tsup::Env g({0.5, 0.5});
    CHECK_EQ(seerTakeCount(g.ctx, 5), 2);  // round_half_even(2.5)=2
    tsup::Env h({0.5, 0.5});
    CHECK_EQ(seerTakeCount(h.ctx, 3), 2);  // round_half_even(1.5)=2
}

// Gold: deposit needs 0 < amount <= hand; withdraw needs amount <= bank.
TEST(bank_deposit_withdraw_beeps) {
    tsup::Env env({0.5, 0.9});
    Purse p;
    p.goldOnHand = 100;
    p.goldInBank = 0;

    CHECK(depositGold(env.ctx, p, 50) == BankResult::Ok);
    CHECK_EQ(p.goldOnHand, 50);
    CHECK_EQ(p.goldInBank, 50);

    CHECK(depositGold(env.ctx, p, 51) == BankResult::Beep);
    CHECK(depositGold(env.ctx, p, 0) == BankResult::Beep);
    CHECK_EQ(p.goldOnHand, 50);

    CHECK(withdrawGold(env.ctx, p, 60) == BankResult::Beep);
    CHECK_EQ(p.goldInBank, 50);
    CHECK(withdrawGold(env.ctx, p, 30) == BankResult::Ok);
    CHECK_EQ(p.goldOnHand, 80);
    CHECK_EQ(p.goldInBank, 20);
}

// transferGold moves min(max(amount,0), from.gold); negative moves nothing.


// Younger: age -= years*365, floored at 16*365 = 5840.
TEST(younger_by_years_with_floor) {
    tsup::Env env({0.5, 0.9});
    Character c;
    c.ageDays = 9000;
    youngerBy(env.ctx, c, 1);
    CHECK_EQ(c.ageDays, 8635);

    c.ageDays = 6000;
    youngerBy(env.ctx, c, 10);   // 2350 < 5840
    CHECK_EQ(c.ageDays, 5840);

    c.ageDays = 6205;
    youngerBy(env.ctx, c, 1);    // lands exactly on the floor
    CHECK_EQ(c.ageDays, 5840);
}

// Combine: overflow when the sum leaves [-32768, 32767]; boundary is allowed.
TEST(combine_charges_overflow_bounds) {
    tsup::Env env({0.5, 0.9});

    // R54: every item in a combine must be fully identified.
    Item a;
    a.type = static_cast<ItemType>(23);
    a.idLevel = IdLevel::All;
    a.charges = 32000;
    Item b;
    b.type = static_cast<ItemType>(23);
    b.idLevel = IdLevel::All;
    b.charges = 767;                      // sum 32767: allowed
    CHECK(checkCombine(env.ctx, a, b) == CombineCheck::Ok);
    combineCharges(env.ctx, a, b);
    CHECK_EQ(a.charges, 32767);
    CHECK_EQ(b.id, 0);

    Item c;
    c.type = static_cast<ItemType>(23);
    c.idLevel = IdLevel::All;
    c.charges = 32000;
    Item d;
    d.type = static_cast<ItemType>(23);
    d.idLevel = IdLevel::All;
    d.charges = 768;                      // sum 32768: overflow
    CHECK(checkCombine(env.ctx, c, d) == CombineCheck::Overflow);

    Item e;
    e.type = static_cast<ItemType>(23);
    e.idLevel = IdLevel::All;
    e.charges = -32000;
    Item f;
    f.type = static_cast<ItemType>(23);
    f.idLevel = IdLevel::All;
    f.charges = -769;                     // sum -32769: overflow
    CHECK(checkCombine(env.ctx, e, f) == CombineCheck::Overflow);

    Item g;
    g.type = static_cast<ItemType>(23);
    g.idLevel = IdLevel::All;
    Item h;
    h.type = static_cast<ItemType>(24);
    h.idLevel = IdLevel::All;
    CHECK(checkCombine(env.ctx, g, h) == CombineCheck::NotSameType);
}

// Re-identify on retrieval: port reads the original count-down test as greater-than.
TEST(reidentify_on_retrieval_reads_greater_than) {
    CHECK(shouldReidentifyOnRetrieval(5, 3));
    CHECK(!shouldReidentifyOnRetrieval(1, 3));
}

// Buy refusals and give/drop refusals for guild-issued items.
TEST(buy_refusals_and_guild_item_give_drop) {
    tsup::Env env({0.5, 0.9});

    Purse p;
    p.goldOnHand = 10;
    Inventory empty;
    Item it;
    it.id = 300;
    CHECK(buyItem(env.ctx, p, empty, it, 40) == BuyResult::NotEnoughGold);
    CHECK_EQ(p.goldOnHand, 10);

    Purse rich;
    rich.goldOnHand = 100;
    Inventory full;
    fillPack(full);
    CHECK(buyItem(env.ctx, rich, full, it, 40) == BuyResult::NoFreeSlot);
    CHECK_EQ(rich.goldOnHand, 100);

    Item guild;
    guild.ailments = kGuildMarkerBase;
    Item normal;
    normal.id = 300;
    CHECK(!canDropSlot(guild));
    CHECK(canDropSlot(normal));

    Inventory recipient;
    CHECK(checkGive(guild, recipient) == GiveRefusal::GuildIssued);
    CHECK(checkGive(normal, recipient) == GiveRefusal::None);

    Inventory fullRecipient;
    fillPack(fullRecipient);
    CHECK(checkGive(normal, fullRecipient) == GiveRefusal::NoFreeSlot);
}
