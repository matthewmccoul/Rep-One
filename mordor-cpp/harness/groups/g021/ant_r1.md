The reference implements this rule (`Party.PartyDeposit`, with `Character.Receive` as the per-member primitive), so I'm writing the three files. Three assumptions need checking before this runs:

- **`VbCurrency` construction is not in the shown API.** `Case.cs` needs to build amounts from gold×10000 units to set up member state, and `Amt()` throws until someone supplies the real constructor or parser. Everything else uses only the shown methods.
- **Reading `VbCurrency` back as units** goes through `Str()`, assuming it renders plain decimal with up to four places. If it doesn't, `Units()` must change.
- **Canonical return shape.** The C# method returns the moved total, the C++ `partyDepositGold` returns a bool, and `partyDepositOnHand` returns a total. Every side therefore emits `ok|total|member states`. For `partyDepositGold` the total is the saturated on-hand sum, which the reference reports on success.

Known divergences the cases are built to expose, not hide: `partyDepositOnHand` skips negative on-hand amounts where the reference moves them, `partyDepositGold` does not roll back members already processed when a later member overflows, and `partyDepositOnHand` has no overflow check, so the max-bank cases hit signed overflow there.

=== FILE: cases.json ===
[
  {"args": {"n": 1, "oh0": 1234500, "bk0": 5000000}, "rng": [0.5]},
  {"args": {"n": 2, "oh0": 100000, "bk0": 0, "oh1": 250000, "bk1": 30000}, "rng": [0.5]},
  {"args": {"n": 4, "oh0": 10000, "bk0": 0, "oh1": 20000, "bk1": 0, "oh2": 30000, "bk2": 0, "oh3": 40000, "bk3": 0}, "rng": [0.5]},
  {"args": {"n": 0}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": 0, "bk0": 0}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": 0, "bk0": 70000}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": 70000, "bk0": 0}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": 1, "bk0": 0}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": -10000, "bk0": 50000}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": -1, "bk0": 0}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": 1, "bk0": 9223372036854775807}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": 5, "bk0": 9223372036854775802}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": 6, "bk0": 9223372036854775802}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": 0, "bk0": 9223372036854775807}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": -1, "bk0": 9223372036854775807}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": 9223372036854775807, "bk0": 0}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": 9223372036854775807, "bk0": 1}, "rng": [0.5]},
  {"args": {"n": 2, "oh0": 100, "bk0": 0, "oh1": 1, "bk1": 9223372036854775807}, "rng": [0.5]},
  {"args": {"n": 3, "oh0": 0, "bk0": 0, "oh1": 0, "bk1": 0, "oh2": 0, "bk2": 0}, "rng": [0.5]},
  {"args": {"n": 4, "oh0": 0, "bk0": 100, "oh1": 500, "bk1": 0, "oh2": 0, "bk2": 700, "oh3": 9000, "bk3": 0}, "rng": [0.5]},
  {"args": {"n": 2, "oh0": 1000000000000, "bk0": 0, "oh1": 1000000000000, "bk1": 0}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": 3000, "bk0": -5000}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": 10000, "bk0": -10000}, "rng": [0.5]},
  {"args": {"n": 2, "oh0": -500, "bk0": 0, "oh1": 800, "bk1": 0}, "rng": [0.5]},
  {"args": {"n": 4, "oh0": 1, "bk0": 0, "oh1": 1, "bk1": 0, "oh2": 1, "bk2": 0, "oh3": 1, "bk3": 9223372036854775807}, "rng": [0.5]},
  {"args": {"n": 1, "oh0": -9223372036854775807, "bk0": 0}, "rng": [0.5]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(JsonElement a, Random rng)
    {
        int n = (int)a.GetProperty("n").GetInt64();
        var members = new List<Character>();
        for (int i = 0; i < n; i++)
        {
            var c = Character.Create("M" + i, RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
            c.GoldOnHand = Amt(a.GetProperty("oh" + i).GetInt64());
            c.GoldInBank = Amt(a.GetProperty("bk" + i).GetInt64());
            members.Add(c);
        }

        // Overflow predicate of the reference: every member's bank + on-hand must fit.
        bool ok = members.All(m => VbCurrency.TryAdd(m.GoldInBank, m.GoldOnHand, out _));

        VbCurrency moved = Party.PartyDeposit(members);

        var sb = new StringBuilder();
        sb.Append(ok ? 1 : 0).Append('|').Append(Units(moved));
        foreach (var m in members)
        {
            sb.Append('|').Append(Units(m.GoldInBank)).Append('|').Append(Units(m.GoldOnHand));
        }
        return sb.ToString();
    }

    // UNKNOWN: VbCurrency has no shown constructor from 1/10000 units. Replace with the real one.
    static VbCurrency Amt(long units) =>
        throw new NotSupportedException("UNKNOWN: VbCurrency construction from gold*10000 units not in shown API");

    // Assumes Str() renders plain decimal with up to 4 places; converts to gold*10000 integer units.
    static long Units(VbCurrency v)
    {
        string s = v.Str().Trim().Replace(",", "");
        decimal d = decimal.Parse(s, NumberStyles.Number | NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture);
        return (long)(d * 10000m);
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include <cstdint>
#include <cstdio>
#include <limits>
#include "case_api.h"
#include "mordor/city1.h"
#include "mordor/partyb.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"

namespace mordor {

static Party buildParty(const JsonValue& a) {
    Party p{};
    p.size = static_cast<i32>(A(a, "n").asInt());
    for (i32 i = 0; i < p.size; ++i) {
        char k[16];
        std::snprintf(k, sizeof k, "oh%d", static_cast<int>(i));
        p.members[i].wallet.goldOnHand = A(a, k).asInt();
        std::snprintf(k, sizeof k, "bk%d", static_cast<int>(i));
        p.members[i].wallet.goldInBank = A(a, k).asInt();
    }
    return p;
}

static Str canon(bool ok, Currency total, const Party& p) {
    Str s = I(ok ? 1 : 0) + "|" + I(total);
    for (i32 i = 0; i < p.size; ++i) {
        s += "|" + I(p.members[i].wallet.goldInBank) + "|" + I(p.members[i].wallet.goldOnHand);
    }
    return s;
}

// Saturating sum of all members' on-hand gold (matches reference VbCurrency.AddSaturating total).
static Currency satSumOnHand(const Party& p) {
    __int128 acc = 0;
    for (i32 i = 0; i < p.size; ++i) acc += p.members[i].wallet.goldOnHand;
    const __int128 hi = static_cast<__int128>(std::numeric_limits<Currency>::max());
    const __int128 lo = static_cast<__int128>(std::numeric_limits<Currency>::min());
    if (acc > hi) acc = hi;
    if (acc < lo) acc = lo;
    return static_cast<Currency>(acc);
}

// The copy returns bool; the total reported is the saturated on-hand sum on success, 0 on failure.
static Str run_partyDepositGold(const JsonValue& a, Ctx& ctx) {
    Party p = buildParty(a);
    Currency expected = satSumOnHand(p);
    bool ok = partyDepositGold(ctx, p);
    return canon(ok, ok ? expected : 0, p);
}

// No overflow check in this copy, so ok is always 1 here.
static Str run_partyDepositOnHand(const JsonValue& a, Ctx& ctx) {
    static_cast<void>(ctx);
    Party p = buildParty(a);
    Currency t = partyDepositOnHand(p);
    return canon(true, t, p);
}

const CopyEntry kCopies[] = {
    {"partyDepositGold", &run_partyDepositGold},
    {"partyDepositOnHand", &run_partyDepositOnHand},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
=== END FILE ===

REFERENCE: Party.PartyDeposit (with Character.Receive as the per-member primitive)