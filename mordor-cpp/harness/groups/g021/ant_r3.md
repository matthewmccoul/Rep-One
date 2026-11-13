The three files are below. The C# side converts integers with the shown implicit `int` conversion and reads amounts back with `Str()`. Both are unverified, so `Case.Run` first checks that `Amt(12345)` prints `1.2345`, `Amt(0)` prints `0.0000`, and `Amt(-5)` prints `-0.0005`. If the unit or format differs, every case fails loudly instead of passing by accident. Two other assumptions are unverified: that `Character.GoldInBank` and `GoldOnHand` are settable, and that the C++ headers are `mordor/partyb.h` and `mordor/city1.h`.

Overflow is reachable without an unknown constructor. Amounts are built by doubling with `TryAdd` on the C# side and `addCurrencyChecked` on the C++ side. The C++ `partyDepositGold` should diverge on cases 17 to 19, where a later member overflows after an earlier member has been changed, since the reference rolls back and the copy does not. Those cases also cover the input-overflow path, the exact `-2^63` boundary, and a near-maximum sum that does not overflow. The `VbCurrency` overflow threshold is unknown, so the C# side may not overflow where the C++ side does. That shows up as a mismatch, which is the intended signal.

=== FILE: cases.json ===
[
 {"args":{"n":0},"rng":[0.5]},
 {"args":{"n":1,"b0":5,"bd0":0,"h0":7,"hd0":0},"rng":[0.5]},
 {"args":{"n":1,"b0":3,"bd0":0,"h0":0,"hd0":0},"rng":[0.5]},
 {"args":{"n":1,"b0":0,"bd0":0,"h0":0,"hd0":0},"rng":[0.5]},
 {"args":{"n":1,"b0":10,"bd0":0,"h0":-4,"hd0":0},"rng":[0.5]},
 {"args":{"n":1,"b0":-10,"bd0":0,"h0":-4,"hd0":0},"rng":[0.5]},
 {"args":{"n":1,"b0":-10,"bd0":0,"h0":4,"hd0":0},"rng":[0.5]},
 {"args":{"n":1,"b0":0,"bd0":0,"h0":1,"hd0":0},"rng":[0.5]},
 {"args":{"n":1,"b0":0,"bd0":0,"h0":10000,"hd0":0},"rng":[0.5]},
 {"args":{"n":1,"b0":0,"bd0":0,"h0":2147483647,"hd0":0},"rng":[0.5]},
 {"args":{"n":1,"b0":0,"bd0":0,"h0":-2147483648,"hd0":0},"rng":[0.5]},
 {"args":{"n":1,"b0":2147483647,"bd0":0,"h0":2147483647,"hd0":0},"rng":[0.5]},
 {"args":{"n":2,"b0":5,"bd0":0,"h0":7,"hd0":0,"b1":0,"bd1":0,"h1":0,"hd1":0},"rng":[0.5]},
 {"args":{"n":2,"b0":0,"bd0":0,"h0":0,"hd0":0,"b1":-3,"bd1":0,"h1":8,"hd1":0},"rng":[0.5]},
 {"args":{"n":4,"b0":1,"bd0":0,"h0":2,"hd0":0,"b1":3,"bd1":0,"h1":4,"hd1":0,"b2":5,"bd2":0,"h2":6,"hd2":0,"b3":7,"bd3":0,"h3":8,"hd3":0},"rng":[0.5]},
 {"args":{"n":3,"b0":0,"bd0":0,"h0":0,"hd0":0,"b1":0,"bd1":0,"h1":0,"hd1":0,"b2":0,"bd2":0,"h2":0,"hd2":0},"rng":[0.5]},
 {"args":{"n":2,"b0":5,"bd0":0,"h0":6,"hd0":0,"b1":1,"bd1":62,"h1":1,"hd1":62},"rng":[0.5]},
 {"args":{"n":2,"b0":1,"bd0":62,"h0":1,"hd0":62,"b1":5,"bd1":0,"h1":6,"hd1":0},"rng":[0.5]},
 {"args":{"n":4,"b0":1,"bd0":0,"h0":2,"hd0":0,"b1":3,"bd1":0,"h1":4,"hd1":0,"b2":5,"bd2":0,"h2":6,"hd2":0,"b3":1,"bd3":62,"h3":1,"hd3":62},"rng":[0.5]},
 {"args":{"n":1,"b0":2147483647,"bd0":32,"h0":2147483647,"hd0":32},"rng":[0.5]},
 {"args":{"n":1,"b0":2147483647,"bd0":32,"h0":2147483647,"hd0":0},"rng":[0.5]},
 {"args":{"n":1,"b0":-1,"bd0":62,"h0":-1,"hd0":62},"rng":[0.5]},
 {"args":{"n":1,"b0":-2147483648,"bd0":33,"h0":0,"hd0":0},"rng":[0.5]},
 {"args":{"n":1,"b0":1,"bd0":63,"h0":0,"hd0":0},"rng":[0.5]},
 {"args":{"n":2,"b0":4,"bd0":0,"h0":4,"hd0":0,"b1":4,"bd1":0,"h1":4,"hd1":0},"rng":[0.1,0.9]},
 {"args":{"n":2,"b0":-7,"bd0":0,"h0":3,"hd0":0,"b1":0,"bd1":0,"h1":-2147483648,"hd1":0},"rng":[0.25]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System;
using System.Collections.Generic;
using System.Text;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    // UNKNOWN: unit of the implicit int conversion. Guarded by the probe in Run.
    static VbCurrency Amt(int units) => units;

    // Uses the shown Str() readback pattern (BankAllText).
    static string Fmt(VbCurrency v) => v.Str().TrimStart(' ');

    // Builds start doubled n times with TryAdd; false on overflow (input-side).
    static bool Build(VbCurrency start, int doublings, out VbCurrency v)
    {
        v = start;
        for (int k = 0; k < doublings; k++)
        {
            if (!VbCurrency.TryAdd(v, v, out var s)) return false;
            v = s;
        }
        return true;
    }

    public static string Run(JsonElement a, Random rng)
    {
        string p1 = Fmt(Amt(12345)), p0 = Fmt(Amt(0)), pn = Fmt(Amt(-5));
        if (p1 != "1.2345" || p0 != "0.0000" || pn != "-0.0005")
            throw new InvalidOperationException(
                "UNIT/FORMAT ASSUMPTION FAILED: Amt(12345)='" + p1 + "' Amt(0)='" + p0 + "' Amt(-5)='" + pn + "'");

        int n = a.GetProperty("n").GetInt32();
        var members = new List<Character>();
        for (int i = 0; i < n; i++)
        {
            if (!Build(Amt(a.GetProperty($"b{i}").GetInt32()), a.GetProperty($"bd{i}").GetInt32(), out VbCurrency bank))
                return "INPUT_OVERFLOW";
            if (!Build(Amt(a.GetProperty($"h{i}").GetInt32()), a.GetProperty($"hd{i}").GetInt32(), out VbCurrency hand))
                return "INPUT_OVERFLOW";

            var c = Character.Create("M" + i, RaceData.ByName("Human"), Alignment.Neutral, Sex.Male,
                                     new int[] { 10, 10, 10, 10, 10, 10 });
            c.GoldInBank = bank;
            c.GoldOnHand = hand;
            members.Add(c);
        }

        var party = new Party(members.ToArray());
        VbCurrency moved = Party.PartyDeposit(party.Members);

        var sb = new StringBuilder();
        sb.Append(Fmt(moved));
        foreach (var c in members)
            sb.Append('|').Append(Fmt(c.GoldInBank)).Append('|').Append(Fmt(c.GoldOnHand));
        return sb.ToString();
    }
}
=== END FILE ===

=== FILE: case.cpp ===
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
=== END FILE ===

REFERENCE: Party.PartyDeposit