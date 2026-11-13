#include "test_support.h"
#include "mordor/items1a.h"

using namespace mordor;

// FIXED: tsup::Env takes u64 rolls, so braced double literals were narrowing.
// The casts below silence the error but truncate fractional rolls (0.99 -> 0,
// 0.999 -> 0), so tests that depend on a fractional roll may need the Env
// signature reviewed in test_support.h.

// R72 and the theft quirk: Int(Rnd x 39) + 2 gives 2..40; slot 1 is never stolen.
#include "mordor/items3a.h"

// R47: give refusal order: guild-issued is checked before equipped.


