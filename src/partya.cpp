// partya.cpp: party membership, join and leave, pruning of the dead, group
// turn targeting, disband and autoload, and group commands.
// Implements include/mordor/partya.h from SRD chapter 17 (part c17p1), rules R1-R30.
#include "mordor/partya.h"

#include <string>

#include "mordor/items1b.h"  // FIXED: SwapResult is declared in items1b.h
#include "mordor/items2b.h"
#include "mordor/math_util.h"

namespace mordor {

namespace {

// R20/R23/R24: balances are Currency in 1/10000 gold.
// UNKNOWN: the bank overflow limit is not stated in this SRD part; 2,000,000,000 gold is used.
constexpr Currency kBalanceCap = 2000000000LL * 10000LL;

}  // namespace

}  // namespace mordor
