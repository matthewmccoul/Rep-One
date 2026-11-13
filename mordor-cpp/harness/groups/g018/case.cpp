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
