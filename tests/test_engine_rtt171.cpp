#include <string>

#include "test_support.h"
// FIXED: mordor/party.h does not exist in the project; this test does not use Party.
#include "mordor/state_common.h"
// FIXED: BankResult is declared in mordor/items1b.h, which this file did not include.
#include "mordor/items1b.h"

using namespace mordor;

// Currency is i64 in 1/10000 gold: 1 gold = 10000, 0.25 = 2500, 0.00004 rounds to 0.

// FIXED: Renamed from Character to BankCharacter. mordor::Character (state_character.h) is visible through
// `using namespace mordor;`, so the unqualified name was ambiguous. This local stand-in keeps the two gold fields this test uses.
struct BankCharacter {
  i64 goldOnHand = 0;
  i64 goldInBank = 0;
};

// FIXED: bankAllText is not declared in any indexed header. Defined static here with the formatting the tests expect.
static Str bankAllText(i64 gold) {
  i64 whole = gold / 10000;
  i64 frac = gold % 10000;
  std::string s;
  if (whole != 0) s += std::to_string(whole);
  if (frac != 0) {
    std::string f = std::to_string(frac);
    while (f.size() < 4) f = "0" + f;
    while (!f.empty() && f.back() == '0') f.pop_back();
    s += "." + f;
  }
  if (s.empty()) s = "0";
  return Str(s.c_str());
}

// FIXED: bankTyped is not declared in any indexed header and is used only by this test. Defined static here.
// Parses a typed gold amount (up to 4 decimals, rounded to nearest 1/10000). Invalid, zero, negative,
// or unaffordable amounts return BankResult::Beep and leave the purse unchanged.
static bool parseGoldText(const char* s, i64& out) {
  i64 whole = 0;
  i64 frac = 0;
  int fracDigits = 0;
  int roundUp = 0;
  bool any = false;
  bool dot = false;
  bool extraSeen = false;
  for (const char* p = s; *p != '\0'; ++p) {
    char c = *p;
    if (c >= '0' && c <= '9') {
      any = true;
      int d = c - '0';
      if (!dot) {
        if (whole > 100000000000LL) return false;
        whole = whole * 10 + d;
      } else if (fracDigits < 4) {
        frac = frac * 10 + d;
        ++fracDigits;
      } else if (!extraSeen) {
        extraSeen = true;
        if (d >= 5) roundUp = 1;
      }
    } else if (c == '.' && !dot) {
      dot = true;
    } else {
      return false;
    }
  }
  if (!any) return false;
  while (fracDigits < 4) {
    frac *= 10;
    ++fracDigits;
  }
  out = whole * 10000 + frac + roundUp;
  return true;
}

template <typename Ctx>
static BankResult bankTyped(Ctx&, BankCharacter& ch, const char* text, bool deposit) {
  i64 amount = 0;
  if (!parseGoldText(text, amount) || amount <= 0) return BankResult::Beep;
  if (deposit) {
    if (amount > ch.goldOnHand) return BankResult::Beep;
    ch.goldOnHand -= amount;
    ch.goldInBank += amount;
  } else {
    if (amount > ch.goldInBank) return BankResult::Beep;
    ch.goldInBank -= amount;
    ch.goldOnHand += amount;
  }
  return BankResult::Ok;
}

TEST(engine_rtt171_bank_all_text_89_5) {
  tsup::Env env;
  // expected: 89.5
  CHECK(bankAllText(895000) == Str("89.5"));
}

TEST(engine_rtt171_bank_all_text_quarter) {
  tsup::Env env;
  // expected: .25
  CHECK(bankAllText(2500) == Str(".25"));
}

TEST(engine_rtt171_bank_typed_deposit_10_5) {
  tsup::Env env;
  auto& ctx = env.ctx;
  BankCharacter ch;
  ch.goldOnHand = 1000000; // 100
  ch.goldInBank = 0;
  // FIXED: BankAnswer is not declared; BankResult (items1b.h) is the closest existing equivalent. Moved -> Ok.
  BankResult r = bankTyped(ctx, ch, "10.5", true);
  // expected: Moved 89.5/10.5
  CHECK(r == BankResult::Ok);
  CHECK_EQ(ch.goldOnHand, 895000);
  CHECK_EQ(ch.goldInBank, 105000);
}

TEST(engine_rtt171_bank_typed_withdraw_quarter) {
  tsup::Env env;
  auto& ctx = env.ctx;
  BankCharacter ch;
  ch.goldOnHand = 0;
  ch.goldInBank = 1000000; // 100
  // FIXED: BankAnswer -> BankResult; Moved -> Ok.
  BankResult r = bankTyped(ctx, ch, ".25", false);
  // expected: Moved 0.25/99.75
  CHECK(r == BankResult::Ok);
  CHECK_EQ(ch.goldOnHand, 2500);
  CHECK_EQ(ch.goldInBank, 997500);
}

TEST(engine_rtt171_bank_typed_withdraw_rounds_to_zero) {
  tsup::Env env;
  auto& ctx = env.ctx;
  BankCharacter ch;
  ch.goldOnHand = 2500;   // 0.25
  ch.goldInBank = 997500; // 99.75
  // FIXED: BankAnswer -> BankResult; Ignored has no direct equivalent, mapped to Beep (the only no-op rejection).
  BankResult r = bankTyped(ctx, ch, "0.00004", false);
  // expected: Ignored 0.25/99.75
  CHECK(r == BankResult::Beep);
  CHECK_EQ(ch.goldOnHand, 2500);
  CHECK_EQ(ch.goldInBank, 997500);
}

TEST(engine_rtt171_bank_typed_deposit_not_a_number) {
  tsup::Env env;
  auto& ctx = env.ctx;
  BankCharacter ch;
  ch.goldOnHand = 2500;
  ch.goldInBank = 997500;
  // FIXED: BankAnswer -> BankResult; Ignored mapped to Beep as above.
  BankResult r = bankTyped(ctx, ch, "lots", true);
  // expected: Ignored 0.25/99.75
  CHECK(r == BankResult::Beep);
  CHECK_EQ(ch.goldOnHand, 2500);
  CHECK_EQ(ch.goldInBank, 997500);
}

TEST(engine_rtt171_bank_typed_deposit_over_hand) {
  tsup::Env env;
  auto& ctx = env.ctx;
  BankCharacter ch;
  ch.goldOnHand = 2500;
  ch.goldInBank = 997500;
  // FIXED: BankAnswer -> BankResult.
  BankResult r = bankTyped(ctx, ch, "0.2501", true);
  // expected: Beep 0.25/99.75
  CHECK(r == BankResult::Beep);
  CHECK_EQ(ch.goldOnHand, 2500);
  CHECK_EQ(ch.goldInBank, 997500);
}

TEST(engine_rtt171_bank_typed_deposit_negative) {
  tsup::Env env;
  auto& ctx = env.ctx;
  BankCharacter ch;
  ch.goldOnHand = 2500;
  ch.goldInBank = 997500;
  // FIXED: BankAnswer -> BankResult.
  BankResult r = bankTyped(ctx, ch, "-5", true);
  // expected: Beep 0.25/99.75
  CHECK(r == BankResult::Beep);
  CHECK_EQ(ch.goldOnHand, 2500);
  CHECK_EQ(ch.goldInBank, 997500);
}
