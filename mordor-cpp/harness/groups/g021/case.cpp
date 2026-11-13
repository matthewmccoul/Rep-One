#include <cstddef>
#include <cstdint>
#include <cstdio>
#include "case_api.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"
#include "mordor/partyb.h"
#include "mordor/city1.h"

namespace mordor {
namespace {

const char* const kBankKey[4] = {"b0", "b1", "b2", "b3"};
const char* const kBankDbl[4] = {"bd0", "bd1", "bd2", "bd3"};
const char* const kHandKey[4] = {"h0", "h1", "h2", "h3"};
const char* const kHandDbl[4] = {"hd0", "hd1", "hd2", "hd3"};

// Doubles start n times with addCurrencyChecked; false on overflow (input-side).
bool doubledCurrency(Currency start, i32 n, Currency& out) {
    Currency x = start;
    for (i32 k = 0; k < n; ++k) {
        Currency s = 0;
        if (!addCurrencyChecked(x, x, s)) return false;
        x = s;
    }
    out = x;
    return true;
}

// Gold text with 4 decimals (assumed to match C# Str(); see Case.Run probe).
Str goldText(Currency u) {
    const bool neg = u < 0;
    const unsigned long long mag = neg
        ? 0ULL - static_cast<unsigned long long>(u)
        : static_cast<unsigned long long>(u);
    char b[64];
    std::snprintf(b, sizeof b, "%s%llu.%04llu", neg ? "-" : "",
                  mag / 10000ULL, mag % 10000ULL);
    return Str(b);
}

// Saturating add, used only to canonicalise the moved total.
Currency addSat(Currency a, Currency b) {
    if (b > 0 && a > INT64_MAX - b) return INT64_MAX;
    if (b < 0 && a < INT64_MIN - b) return INT64_MIN;
    return a + b;
}

bool buildParty(const JsonValue& a, Party& party) {
    party.size = static_cast<i32>(A(a, "n").asInt());
    for (i32 i = 0; i < party.size; ++i) {
        Currency bank = 0;
        Currency hand = 0;
        if (!doubledCurrency(static_cast<Currency>(A(a, kBankKey[i]).asInt()),
                             static_cast<i32>(A(a, kBankDbl[i]).asInt()), bank)) {
            return false;
        }
        if (!doubledCurrency(static_cast<Currency>(A(a, kHandKey[i]).asInt()),
                             static_cast<i32>(A(a, kHandDbl[i]).asInt()), hand)) {
            return false;
        }
        Character& m = party.members[static_cast<u32>(i)];
        m.wallet.goldInBank = bank;
        m.wallet.goldOnHand = hand;
    }
    return true;
}

Str stateText(const Party& party, const Str& head) {
    Str out = head;
    for (i32 i = 0; i < party.size; ++i) {
        const Character& m = party.members[static_cast<u32>(i)];
        out += "|" + goldText(m.wallet.goldInBank) + "|" + goldText(m.wallet.goldOnHand);
    }
    return out;
}

static Str run_partyDepositGold(const JsonValue& a, Ctx& ctx) {
    Party party{};
    if (!buildParty(a, party)) return Str("INPUT_OVERFLOW");
    // Canonical moved total: saturating sum of on-hand when the copy reports success, else zero.
    Currency sum = 0;
    for (i32 i = 0; i < party.size; ++i) {
        sum = addSat(sum, party.members[static_cast<u32>(i)].wallet.goldOnHand);
    }
    const bool ok = partyDepositGold(ctx, party);
    const Currency moved = ok ? sum : 0;
    return stateText(party, goldText(moved));
}

static Str run_partyDepositOnHand(const JsonValue& a, Ctx& ctx) {
    static_cast<void>(ctx);
    Party party{};
    if (!buildParty(a, party)) return Str("INPUT_OVERFLOW");
    const Currency total = partyDepositOnHand(party);
    return stateText(party, goldText(total));
}

}  // namespace

const CopyEntry kCopies[] = {
    {"partyDepositGold", &run_partyDepositGold},
    {"partyDepositOnHand", &run_partyDepositOnHand},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
