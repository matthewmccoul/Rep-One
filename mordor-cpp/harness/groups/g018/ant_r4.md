=== FILE: cases.json ===
[
  {"args": {"giver_gold": 100, "receiver_gold": 50, "amount": 30}, "rng": [0.5]},
  {"args": {"giver_gold": 30, "receiver_gold": 0, "amount": 30}, "rng": [0.5]},
  {"args": {"giver_gold": 29, "receiver_gold": 0, "amount": 30}, "rng": [0.5]},
  {"args": {"giver_gold": 0, "receiver_gold": 0, "amount": 1}, "rng": [0.5]},
  {"args": {"giver_gold": 100, "receiver_gold": 50, "amount": 0}, "rng": [0.5]},
  {"args": {"giver_gold": 100, "receiver_gold": 50, "amount": -5}, "rng": [0.5]},
  {"args": {"giver_gold": 1, "receiver_gold": 0, "amount": 1}, "rng": [0.5]},
  {"args": {"giver_gold": 500, "receiver_gold": 500, "amount": 500}, "rng": [0.5]},
  {"args": {"giver_gold": 1000000, "receiver_gold": 2500, "amount": 999999}, "rng": [0.5]},
  {"args": {"giver_gold": 7, "receiver_gold": 922337203685476, "amount": 1}, "rng": [0.5]},
  {"args": {"giver_gold": 7, "receiver_gold": 922337203685476, "amount": 2}, "rng": [0.5]},
  {"args": {"giver_gold": 10, "receiver_gold": 922337203685477, "amount": 1}, "rng": [0.5]},
  {"args": {"giver_gold": 0, "receiver_gold": 922337203685477, "amount": 1}, "rng": [0.5]},
  {"args": {"giver_gold": 922337203685477, "receiver_gold": 0, "amount": 922337203685477}, "rng": [0.5]},
  {"args": {"giver_gold": 30000000000000, "receiver_gold": 900000000000000, "amount": 30000000000000}, "rng": [0.5]},
  {"args": {"giver_gold": 2147483647, "receiver_gold": 2147483647, "amount": 2147483647}, "rng": [0.5]},
  {"args": {"giver_gold": 10, "receiver_gold": 0, "amount": 9999999999}, "rng": [0.5]},
  {"args": {"giver_gold": 0, "receiver_gold": 0, "amount": -1}, "rng": [0.5]},
  {"args": {"giver_gold": 100, "receiver_gold": 0, "amount": -100}, "rng": [0.5]},
  {"args": {"giver_gold": 5, "receiver_gold": 0, "amount": 5}, "rng": [0.5]},
  {"args": {"giver_gold": 5, "receiver_gold": 922337203685476, "amount": 0}, "rng": [0.5]},
  {"args": {"giver_gold": 922337203685477, "receiver_gold": 922337203685476, "amount": 1}, "rng": [0.5]},
  {"args": {"giver_gold": 1, "receiver_gold": 1, "amount": 1}, "rng": [0.5]}
]
=== END FILE ===

=== FILE: Case.cs ===
using System;
using System.Globalization;
using System.Linq;
using System.Text.Json;
using Mordor.Core;

public static class Case
{
    public static string Run(System.Text.Json.JsonElement a, System.Random rng)
    {
        long giverStart = a.GetProperty("giver_gold").GetInt64();
        long receiverStart = a.GetProperty("receiver_gold").GetInt64();
        long amount = a.GetProperty("amount").GetInt64();

        var giver = Character.Create("Giver", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
        var receiver = Character.Create("Receiver", RaceData.ByName("Human"), Alignment.Neutral, Sex.Male, new int[6]);
        SetGold(giver, giverStart);
        SetGold(receiver, receiverStart);

        var said = GiveCommand.Give(giver, receiver, 0, amount, _ => null);
        int ret = said.Count > 0 ? 1 : 0;

        return ret.ToString(CultureInfo.InvariantCulture) + "|"
             + WholeGold(giver.GoldOnHand).ToString(CultureInfo.InvariantCulture) + "|"
             + WholeGold(receiver.GoldOnHand).ToString(CultureInfo.InvariantCulture);
    }

    // Core's Character.Create sets GoldOnHand = 1500 and SetGold adds to it.
    // Remove the starting 1500 first so the balance equals the case value exactly.
    static void SetGold(Character c, long gold)
    {
        if (!VbCurrency.TrySubtract(c.GoldOnHand, 1500L, out var zero))
            throw new InvalidOperationException("setup reset failed");
        c.GoldOnHand = zero;

        if (gold <= 0) return;
        if (!VbCurrency.TryAdd(c.GoldOnHand, gold, out var v))
            throw new InvalidOperationException("setup overflow");
        c.GoldOnHand = v;
    }

    static long WholeGold(VbCurrency v)
    {
        string digits = new string(v.Str().Where(ch => char.IsDigit(ch) || ch == '.' || ch == '-').ToArray());
        return (long)Math.Floor(decimal.Parse(digits, NumberStyles.Float, CultureInfo.InvariantCulture));
    }
}
=== END FILE ===

=== FILE: case.cpp ===
#include "case_api.h"
#include "mordor/attributes.h"
#include "mordor/state_character.h"
#include "mordor/items2b.h"
#include "mordor/loot1a.h"
#include "mordor/partya.h"

namespace mordor {

// Same-name copies. The harness renames each definition in its own source file.
GiveResult giveGold__items2b(Ctx& ctx, Character& giver, Character& receiver,
                             i32 giverIndex, i32 receiverIndex, i64 wholeGold, bool inCombat);
bool giveGold__loot1a(Ctx& ctx, Character& giver, Character& receiver, i64 wholeGold);

// Build a character the same way Core's Character.Create does: Human, Nomad, level 1,
// all six stats zero (matches new int[6] on the C# side).
static void makeChar(Ctx& ctx, Character& c) {
    createCharacter(ctx, c, Race::Human, Guild::Nomad, {});
}

static void setGold(Character& c, i64 gold) {
    // Overwrite the starting purse so the balance equals the case value exactly.
    c.wallet.goldOnHand = currencyFromWhole(gold);
}

static Str state(const Character& giver, const Character& receiver) {
    return I(currencyWhole(giver.wallet.goldOnHand)) + "|" + I(currencyWhole(receiver.wallet.goldOnHand));
}

static Str run_giveGold_items2b(const JsonValue& a, Ctx& ctx) {
    Character giver;
    Character receiver;
    makeChar(ctx, giver);
    makeChar(ctx, receiver);
    setGold(giver, A(a, "giver_gold").asInt());
    setGold(receiver, A(a, "receiver_gold").asInt());
    const i64 amount = A(a, "amount").asInt();
    const GiveResult r = giveGold__items2b(ctx, giver, receiver, 0, 1, amount, false);
    return I(r == GiveResult::Done ? 1 : 0) + "|" + state(giver, receiver);
}

static Str run_giveGold_loot1a(const JsonValue& a, Ctx& ctx) {
    Character giver;
    Character receiver;
    makeChar(ctx, giver);
    makeChar(ctx, receiver);
    setGold(giver, A(a, "giver_gold").asInt());
    setGold(receiver, A(a, "receiver_gold").asInt());
    const i64 amount = A(a, "amount").asInt();
    const bool ok = giveGold__loot1a(ctx, giver, receiver, amount);
    return I(ok ? 1 : 0) + "|" + state(giver, receiver);
}

static Str run_giveGoldTransfer(const JsonValue& a, Ctx& ctx) {
    Character giver;
    Character receiver;
    makeChar(ctx, giver);
    makeChar(ctx, receiver);
    setGold(giver, A(a, "giver_gold").asInt());
    setGold(receiver, A(a, "receiver_gold").asInt());
    const i64 amount = A(a, "amount").asInt();
    const GiveGoldOutcome o = giveGoldTransfer(ctx, giver, receiver, currencyFromWhole(amount));
    return I(o == GiveGoldOutcome::Done ? 1 : 0) + "|" + state(giver, receiver);
}

const CopyEntry kCopies[] = {
    {"giveGold@items2b.cpp", &run_giveGold_items2b},
    {"giveGold@loot1a.cpp", &run_giveGold_loot1a},
    {"giveGoldTransfer", &run_giveGoldTransfer},
};
const std::size_t kNumCopies = 3;

}  // namespace mordor
=== END FILE ===

Changes from the previous attempt:
- **Case.cs:** `SetGold` first subtracts the starting 1500 with `VbCurrency.TrySubtract`, then adds the case value. The balance now equals the case value exactly. `TrySubtract` is the same call `GiveCommand.Give` already uses, so no new API is assumed.
- **case.cpp:** every Character is built with `createCharacter(ctx, c, Race::Human, Guild::Nomad, {})`. The `{}` gives six zero stats, matching `new int[6]` on the C# side. The gold is then assigned exactly, so the starting purse doesn't affect the result.
- **case.cpp:** the `kCopies` table is now complete and `kNumCopies = 3`. The previous file was cut off at `kNumCo`.

Unverified assumptions:
- The `createCharacter` parameter types are inferred from the call form you gave. I did not see the header, so the `{}` for the stats argument needs to compile against the real signature.
- The C# `WholeGold` helper and the cases with receiver balances near 922337203685477 may throw during setup (`setup overflow`) if that exceeds the cap. I did not change this.

REFERENCE: GiveCommand.Give