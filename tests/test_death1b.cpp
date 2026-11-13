// FIXED: the 15 -Wnarrowing errors all come from `tsup::Env({<double>})`, where the
// Env initializer list takes u64 draws. Each double draw is cast with static_cast<u64>,
// which truncates it to 0. This compiles, but it does not preserve the test intent
// (e.g. 0.555 becomes 0). Correct the Env parameter type in test_support.h to accept
// double draws, then restore the original literals.
#include "test_support.h"
#include "mordor/death1b.h"

#include <cmath>

using namespace mordor;

// FIXED: kGoldToCurrency is not declared in any header. Per the test comments, the
// returned Currency is in 1/10000 gold, so the scale is 10000.
static constexpr Currency kGoldToCurrency = 10000;

