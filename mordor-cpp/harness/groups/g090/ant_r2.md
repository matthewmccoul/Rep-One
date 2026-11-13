The exception came from `Convert.ToDouble` on `GoldOnHand` and `GoldInBank`, which are `VbCurrency` values rather than numbers. The fix reads each balance through `RoundToWhole()`, the same method the reference's `Pay` already calls on a `VbCurrency` (`owed.RoundToWhole()`), and passes the resulting `long` to `H.F`. I haven't compiled or run the harness, so treat this as unverified until it runs.

=== FILE: cases.json ===
[
{"args":{"cost":0,"onHand":100,"bank":50},"rng":[0.5]},
{"args":{"cost":10,"onHand":100,"bank":50},"rng":[0.5]},
{"args":{"cost":100,"onHand":100,"bank":50},"rng":[0.5]},
{"args":{"cost":101,"onHand":100,"bank":50},"rng":[0.5]},
{"args":{"cost":150,"onHand":100,"bank":50},"rng":[0.5]},
{"args":{"cost":151,"onHand":100,"bank":50},"rng":[0.5]},
{"args":{"cost":1,"onHand":0,"bank":0},"rng":[0.5]},
{"args":{"cost":0,"onHand":0,"bank":0},"rng":[0.5]},
{"args":{"cost":-5,"onHand":100,"bank":50},"rng":[0.5]},
{"args":{"cost":-1,"onHand":0,"bank":0},"rng":[0.5]},
{"args":{"cost":200,"onHand":0,"bank":500},"rng":[0.5]},
{"args":{"cost":200,"onHand":500,"bank":0},"rng":[0.5]},
{"args":{"cost":50,"onHand":0,"bank":50},"rng":[0.5]},
{"args":{"cost":51,"onHand":0,"bank":50},"rng":[0.5]},
{"args":{"cost":31,"onHand":30,"bank":0},"rng":[0.5]},
{"args":{"cost":30,"onHand":30,"bank":0},"rng":[0.5]},
{"args":{"cost":1,"onHand":1,"bank":0},"rng":[0.5]},
{"args":{"cost":922337203685477,"onHand":0,"bank":922337203685477},"rng":[0.5]},
{"args":{"cost":922337203685477,"onHand":0,"bank":922337203685476},"rng":[0.5]},
{"args":{"cost":922337203685477,"onHand":922337203685477,"bank":0},"rng":[0.5]},
{"args":{"cost":1,"onHand":922337203685477,"bank":0},"rng":[0.5]},
{"args":{"cost":1000000000,"onHand":10,"bank":10},"rng":[0.5]},
{"args":{"cost":9,"onHand":7,"bank":3},"rng":[0.5]},
{"args":{"cost":10,"onHand":7,"bank":3},"rng":[0.5]},
{"args":{"cost":11,"onHand":7,"bank":3},"rng":[0.5]},
{"args":{"cost":-1000000,"onHand":5,"bank":5},"rng":[0.5]},
{"args":{"cost":1,"onHand":0,"bank":1},"rng":[0.5]},
{"args":{"cost":2,"onHand":1,"bank":1},"rng":[0.5]},
{"args":{"cost":3,"onHand":1,"bank":1},"rng":[0.5]},
{"args":{"cost":0,"onHand":0,"bank":5},"rng":[0.5]},
{"args":{"cost":5,"onHand":0,"bank":0},"rng":[0.5]}
]
=== END FILE ===
=== FILE: Case.cs ===
using System;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    // Reference: Character.Pay(long cost). Costs are whole gold, matching the C++ units (gold x 10000).
    public static string Run(JsonElement a, Random rng)
    {
        long cost = a.GetProperty("cost").GetInt64();
        long onHand = a.GetProperty("onHand").GetInt64();
        long bank = a.GetProperty("bank").GetInt64();

        var c = Character.Create("Tester", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
        c.GoldOnHand = onHand;
        c.GoldInBank = bank;

        bool ok = c.Pay(cost);

        // GoldOnHand/GoldInBank are VbCurrency; read them with RoundToWhole() (as Pay does) to get whole gold.
        return string.Join("|", new[]
        {
            ok ? "1" : "0",
            H.F(c.GoldOnHand.RoundToWhole()),
            H.F(c.GoldInBank.RoundToWhole()),
        });
    }
}
=== END FILE ===
=== FILE: case.cpp ===
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
=== END FILE ===

REFERENCE: Character.Pay