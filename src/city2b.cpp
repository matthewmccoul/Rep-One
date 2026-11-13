#include "mordor/city2b.h"
#include "mordor/city1.h"
#include "mordor/ctx.h"
#include "mordor/math_util.h"

namespace mordor {

namespace {
// T-L Fee base (seer)
constexpr double kSeerFeeBase = 132567.52;
// T-L Level gate (original) 12, FRMSEEK.BAS:90, 237
constexpr i32 kSeerLevelGate = 12;
// T-J Buy base floor: max(BaseValue, 10)
constexpr i32 kBuyFloor = 10;
// T-R companion price band table length
constexpr i32 kCompanionBandCount =
    static_cast<i32>(sizeof(kCompanionPriceBands) / sizeof(kCompanionPriceBands[0]));
}  // namespace

// R74 ratio_raw = Price(numerator monster) / Price(denominator monster)
// UNKNOWN: zero denominator not specified in SRD; returns 0.0.
double ratioRaw(Currency priceNumerator, Currency priceDenominator) {
    if (priceDenominator == 0) {
        return 0.0;
    }
    return static_cast<double>(priceNumerator) / static_cast<double>(priceDenominator);
}

// UNKNOWN: rest gate rule not present in SRD chunks. Simplest sensible choice:
// a dead character cannot rest; dungeonLevel does not restrict the rest.
bool cityRestGate(i32 dungeonLevel, bool dead) {
    (void)dungeonLevel;
    return !dead;
}

// R3 / T-K rest days:
//   numerator = 100 * (missing fraction of hits)
//   days      = round_half_even(numerator / 3.25)
// Checked against T-K examples: 0 hits -> 31 days; a tenth left -> 28 days.
// UNKNOWN: maxHits <= 0 and full-health cases are not stated; both return 0.
i64 cityRestDays(i32 curHits, i32 maxHits) {
    if (maxHits <= 0) {
        return 0;
    }
    i32 missing = maxHits - curHits;
    if (missing <= 0) {
        return 0;
    }
    double numerator = static_cast<double>(kRestNumerator) *
                       static_cast<double>(missing) / static_cast<double>(maxHits);
    return roundHalfEven(numerator / kRestDivisor);
}

// UNKNOWN: alignment-choice prompt condition not stated in SRD chunks.
// Simplest sensible choice: asked only when more than one alignment is stocked.
bool alignmentChoiceAsked(i32 stockedAlignmentCount) {
    return stockedAlignmentCount > 1;
}

// UNKNOWN: shelving gate not fully stated. Uses R16/R21 ShelfCap (9) as the row cap
// and T-J deepest stocked floor (14) as the floor limit. Cursed and unaligned
// items are refused.
bool shelvingAllowed(bool cursed, bool unaligned, i32 floor, i32 alignRowCount) {
    if (cursed || unaligned) {
        return false;
    }
    if (floor > kDeepestStocked) {
        return false;
    }
    return alignRowCount < kShelfCap;
}

// R18 / T-J: DeepestStocked = min(deepestReached, mc0118 - 1) with mc0118 = 15.
i32 cappedShelfDepth(i32 deepestReached) {
    return deepestReached < kDeepestStocked ? deepestReached : kDeepestStocked;
}

// R16/R21 ShelfCap = 9 per pen. The 60% add roll (T-J "rnd*100 < 60") is made
// by the caller, not here. Returns true when one creature was added.
bool penAddOne(i32& penCount) {
    if (penCount >= kShelfCap) {
        return false;
    }
    ++penCount;
    return true;
}


// R26 store realign fee: price * 0.72 (kRealignFactor), rounded half-even.
Currency realignStoreFee(Currency price) {
    return roundHalfEven(static_cast<double>(price) * kRealignFactor);
}

// T-I: vault/pack index 0 and indices >= 40 are refused.
// Accepted: 0 < slot < kBankSlots (40).
bool vaultSlotAccepted(i32 slot) {
    return slot > 0 && slot < kBankSlots;
}

// UNKNOWN: "empty" slot test not given in SRD chunks. An Item with id 0 is treated as empty.
i32 bankFreeSlots(const Inventory& inv) {
    i32 freeCount = 0;
    for (i32 i = 0; i < kBankSlots; ++i) {
        if (inv.bank[static_cast<u32>(i)].id == 0) {
            ++freeCount;
        }
    }
    return freeCount;
}

// UNKNOWN: shortfall rule not stated in SRD chunks. Shortfall = cost - purse, floored at 0.
Currency shortfallNeed(Currency cost, Currency payerPurse) {
    Currency need = cost - payerPurse;
    return need > 0 ? need : 0;
}

// UNKNOWN: payment order not stated in SRD chunks. Takes on-hand gold first, then bank.
// Refuses (no change to purse) when the purse cannot cover cost.
// UNKNOWN: ChargeResult field names are not in the API INDEX, so the result is
// returned default-initialised; the caller must read the purse directly.
ChargeResult charPay(Ctx& ctx, Character& payer, Currency cost) {
    ChargeResult result{};
    if (cost > payer.purse()) {
        ctx.say("city.charge.short", "Not enough gold to pay the charge.",
                {static_cast<i64>(cost)}, {});
        return result;  // result.paid stays false
    }
    Currency fromHand = payer.wallet.goldOnHand < cost ? payer.wallet.goldOnHand : cost;
    payer.wallet.goldOnHand -= fromHand;
    payer.wallet.goldInBank -= (cost - fromHand);
    result.paid = true;  // the charge was covered from hand and bank
    ctx.say("city.charge", "Charge paid.", {static_cast<i64>(cost)}, {});
    return result;
}


// T-H library day text:
//   today == 0 -> NotKnown; days == 0 -> Today; days == 1 -> OneDay; else Days.
LibraryDayPhrase libraryDayPhrase(i64 todaySerial, i64 days) {
    if (todaySerial == 0) {
        return LibraryDayPhrase::NotKnown;
    }
    if (days == 0) {
        return LibraryDayPhrase::Today;
    }
    if (days == 1) {
        return LibraryDayPhrase::OneDay;
    }
    return LibraryDayPhrase::Days;
}

// T-L level gate (original) 12 (FRMSEEK.BAS:90, 237).
// UNKNOWN: how deepestHeCanReach enters the refusal is not stated in the SRD chunks;
// only the level gate is applied here.
bool seerRefuses(i32 level, i32 deepestHeCanReach) {
    (void)deepestHeCanReach;
    return level < kSeerLevelGate;
}

// T-L fee base 132567.52.
// UNKNOWN: level dependence not stated in SRD chunks. Chosen: fee = base * level.
Currency seerFeeBase(i32 level) {
    return roundHalfEven(kSeerFeeBase * static_cast<double>(level));
}

// UNKNOWN: clean-log visibility rule not stated in SRD chunks.
// Simplest sensible choice: visible when there are log entries and at least one guild.
bool guildLogCleanVisible(i32 logEntryCount, i32 guildCount) {
    return logEntryCount > 0 && guildCount > 0;
}

// Currency is i64 in 1/10000 gold (types.h), so gold * 10000.
// UNKNOWN: negative input not specified; clamped to 0.
Currency goldInputToCurrency(double enteredGold) {
    if (enteredGold < 0.0) {
        return 0;
    }
    return roundHalfEven(enteredGold * 10000.0);
}

// MaxWorn class wear maximum [c63].
// UNKNOWN: maxWorn < 0 treated as no limit.
// UNKNOWN: the refusal enum member name is truncated in items3a.h; value 1 is used
// as the max-worn refusal. Replace with the named member when items3a.h is confirmed.
WearRefusal classWearCheck(i32 maxWorn, i32 wornCount) {
    if (maxWorn < 0) {
        return WearRefusal::None;
    }
    if (wornCount >= maxWorn) {
        return static_cast<WearRefusal>(1);
    }
    return WearRefusal::None;
}

// T-M item alignment vs pen index:
//   Good (1) -> 0, Neutral (2) -> 1, Evil (3) -> 2.
// UNKNOWN: Unaligned has no pen row; returns -1.
i32 penAlignIndex(ItemAlign align) {
    switch (align) {
        case ItemAlign::Good:
            return 0;
        case ItemAlign::Neutral:
            return 1;
        case ItemAlign::Evil:
            return 2;
        default:
            return -1;
    }
}

// T-R companion price band for levelFound. Index = level - 1.
// UNKNOWN: out-of-range levels are clamped to the table ends.
CompanionPriceBand companionPriceBand(i32 level) {
    i32 idx = level - 1;
    if (idx < 0) {
        idx = 0;
    }
    if (idx > kCompanionBandCount - 1) {
        idx = kCompanionBandCount - 1;
    }
    return kCompanionPriceBands[idx];
}

// CONFLICT: drawRoundedIndex is also declared in city1.h (as roundedIndex) and in
// city3a.h with the same signature. Defined once, here, because city2b.h declares it.
// UNKNOWN: rounding rule not stated in SRD chunks. Chosen: round_half_even of
// rnd * (n - 1), giving an index in 0..n-1 from ctx.rng only.
// SRD quirk "Rounded index bias": index = VB Int-style rounding of Rnd * n.
// 0 and n each get half weight, so an index equal to n matches nothing and is retried.
// Uses ctx.rng.nextDouble() directly, per the RNG convention.
i32 drawRoundedIndex(Ctx& ctx, i32 n) {
    if (n <= 1) {
        return 0;
    }
    for (;;) {
        double r = ctx.rng.nextDouble() * static_cast<double>(n);
        i64 idx = roundHalfEven(r);
        if (idx < static_cast<i64>(n)) {
            return static_cast<i32>(idx);
        }
        // Index equal to n: retry (SRD quirk, no clamp).
    }
}

}  // namespace mordor
