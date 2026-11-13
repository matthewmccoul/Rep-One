#include "mordor/ref_goldtosteal.h"

#include <limits>

namespace mordor {

namespace {

// Currency is i64 in 1/10000 gold.
constexpr i64 kCurrencyPerGold = 10000;

// VbCurrency.Saturate: gold amount to Currency, clamped to the Currency range.
Currency saturateGold(i64 gold) {
    const Currency hi = std::numeric_limits<Currency>::max();
    const Currency lo = std::numeric_limits<Currency>::min();
    if (gold > hi / kCurrencyPerGold) return hi;
    if (gold < lo / kCurrencyPerGold) return lo;
    return gold * kCurrencyPerGold;
}

// VbCurrency.RoundToWhole: round to whole gold, half to even (SRD round_half_even).
i64 roundCurrencyToGold(Currency c) {
    i64 whole = c / kCurrencyPerGold;
    i64 rem = c % kCurrencyPerGold;
    if (rem < 0) {  // floor division, so the remainder is in [0, 10000)
        whole -= 1;
        rem += kCurrencyPerGold;
    }
    const i64 half = kCurrencyPerGold / 2;
    if (rem > half) return whole + 1;
    if (rem < half) return whole;
    return (whole % 2 == 0) ? whole : whole + 1;  // exactly .5: to even
}

}  // namespace

bool goldToSteal(Currency onHand, i64 rolled, i64& taken) {
    // R-less: MONSMOD thief take. Left = onHand - Saturate(rolled); overflow means null.
    const Currency sub = saturateGold(rolled);
    const Currency lo = std::numeric_limits<Currency>::min();
    const Currency hi = std::numeric_limits<Currency>::max();

    // VbCurrency.TrySubtract(onHand, sub): fail when the difference leaves the range.
    if (sub >= 0 ? onHand < lo + sub : onHand > hi + sub) return false;

    const Currency left = onHand - sub;
    if (left < 0) {
        // Purse runs out: the thief takes the whole purse, rounded to whole gold.
        taken = roundCurrencyToGold(onHand);
    } else {
        taken = rolled;
    }
    return true;
}

}  // namespace mordor
