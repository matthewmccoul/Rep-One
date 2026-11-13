#include "case_api.h"
#include "mordor/items2b.h"
#include "mordor/companions1a.h"

namespace mordor {
namespace {

constexpr long long kUnitsPerGold = 10000;  // C++ Currency = gold x 10000

Currency goldUnits(long long gold) { return static_cast<Currency>(gold * kUnitsPerGold); }

Str wholeGold(Currency units) { return F(static_cast<double>(static_cast<long long>(units) / kUnitsPerGold)); }

void setWallet(Character& c, const JsonValue& a) {
    c.wallet.goldOnHand = goldUnits(A(a, "onHand").asInt());
    c.wallet.goldInBank = goldUnits(A(a, "bank").asInt());
}

Str canon(bool ok, const Character& c) {
    return I(ok ? 1 : 0) + "|" + wholeGold(c.wallet.goldOnHand) + "|" + wholeGold(c.wallet.goldInBank);
}

}  // namespace

static Str run_chargePurse(const JsonValue& a, Ctx& ctx) {
    (void)ctx;  // chargePurse takes no context
    Character c{};
    setWallet(c, a);
    const bool ok = chargePurse(c, goldUnits(A(a, "cost").asInt()));
    return canon(ok, c);
}

static Str run_payCharacterGold(const JsonValue& a, Ctx& ctx) {
    Character c{};
    setWallet(c, a);
    const bool ok = payCharacterGold(ctx, c, goldUnits(A(a, "cost").asInt()));
    return canon(ok, c);
}

const CopyEntry kCopies[] = {
    {"chargePurse", &run_chargePurse},
    {"payCharacterGold", &run_payCharacterGold},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
