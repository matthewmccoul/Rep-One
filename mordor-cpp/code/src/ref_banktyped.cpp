#include "mordor/ref_banktyped.h"

namespace mordor {
namespace {

constexpr Currency kUnitsPerGold = 10000;
constexpr Currency kCurrencyMax = 9223372036854775807LL;
constexpr Currency kCurrencyMin = -kCurrencyMax - 1;

// True when a + b stays inside the i64 range (VbCurrency.TryAdd).
bool addFits(Currency a, Currency b) {
    if (b > 0) return a <= kCurrencyMax - b;
    return a >= kCurrencyMin - b;
}

// True when a - b stays inside the i64 range (VbCurrency.TrySubtract).
bool subFits(Currency a, Currency b) {
    if (b > 0) return a >= kCurrencyMin + b;
    return a <= kCurrencyMax + b;
}

bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

// VbCurrency.TryParse: optional sign, digits, optional '.' and digits.
// Keeps 4 decimal places, rounding half to even on the exact decimal.
// Anything else, or a magnitude outside the currency range, fails.
// UNKNOWN: the reference's exact handling of leading/trailing blanks is not in the extract; none are accepted here.
bool parseCurrency(const Str& s, Currency& out) {
    const size_t n = s.size();
    size_t i = 0;
    bool neg = false;
    if (i < n && (s[i] == '-' || s[i] == '+')) {
        neg = (s[i] == '-');
        ++i;
    }

    const Currency wholeLimit = kCurrencyMax / kUnitsPerGold;
    Currency whole = 0;
    bool anyDigit = false;
    while (i < n && isDigit(s[i])) {
        const Currency d = s[i] - '0';
        if (whole > (wholeLimit * 10 + 9 - d) / 10 + 1) return false;  // guard before multiplying
        if (whole * 10 + d > wholeLimit) {
            // Only allowed if the final magnitude still fits; checked below via frac budget.
            return false;
        }
        whole = whole * 10 + d;
        anyDigit = true;
        ++i;
    }

    Currency frac = 0;
    int placed = 0;          // fractional digits stored in frac (max 4)
    int nextDigit = -1;      // first digit beyond 4 places, or -1 if none
    bool sticky = false;     // any non-zero digit beyond that
    int count = 0;           // fractional digits seen
    if (i < n && s[i] == '.') {
        ++i;
        while (i < n && isDigit(s[i])) {
            const int d = s[i] - '0';
            if (count < 4) {
                frac = frac * 10 + d;
                ++placed;
            } else if (count == 4) {
                nextDigit = d;
            } else if (d != 0) {
                sticky = true;
            }
            if (count >= 0) anyDigit = true;
            ++count;
            ++i;
        }
    }
    if (i != n || !anyDigit) return false;
    for (int k = placed; k < 4; ++k) frac *= 10;

    // Round half to even on the 5th decimal and beyond.
    bool up = false;
    if (nextDigit > 5) {
        up = true;
    } else if (nextDigit == 5) {
        up = sticky ? true : (frac % 2 == 1);
    }

    // Magnitude = whole*10000 + frac + up, must not exceed kCurrencyMax.
    const Currency rem = kCurrencyMax - whole * kUnitsPerGold;  // >= 0 since whole <= wholeLimit
    const Currency fracPart = frac + (up ? 1 : 0);
    if (fracPart > rem) return false;
    const Currency magnitude = whole * kUnitsPerGold + fracPart;
    out = neg ? -magnitude : magnitude;
    return true;
}

}  // namespace

// Party.cs:280 BankTyped
BankAnswer bankTyped(Character& ch, const Str& text, bool deposit) {
    Currency amount = 0;
    if (!parseCurrency(text, amount) || amount == 0) return BankAnswer::Ignored;

    Purse& w = ch.wallet;
    if (deposit) {
        if (amount > w.goldOnHand || amount <= 0) return BankAnswer::Beep;
        if (addFits(w.goldInBank, amount)) w.goldInBank += amount;      // 315
        if (subFits(w.goldOnHand, amount)) w.goldOnHand -= amount;      // 316
    } else {
        if (amount > w.goldInBank || w.goldInBank < 0) return BankAnswer::Beep;
        if (addFits(w.goldOnHand, amount)) w.goldOnHand += amount;      // 323
        if (subFits(w.goldInBank, amount)) w.goldInBank -= amount;      // 324
    }
    return BankAnswer::Moved;
}

}  // namespace mordor
