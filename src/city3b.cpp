// city3b: store replenish, companion stock pricing, and sighting merge rules.
// Source citations use the SRD chunk tags (e.g. [c9]) because this part of the SRD has no R-numbers.
#include <cmath>

#include "mordor/city3b.h"
#include "mordor/ctx.h"
#include "mordor/math_util.h"
#include "mordor/rng.h"
#include "mordor/types.h"

namespace mordor {

// Store replenish chance. [c22]
// UNKNOWN: `days / 100 * 100` is literal or integer division. Integer division is chosen
// here (the simplest reading that yields a percent-like step); see kStoreReplenishStep.
i32 storeReplenishChance(i32 daysAged) {
  return (daysAged / kStoreReplenishStep) * kStoreReplenishStep;
}

// Roll for store replenish using the chance above. [c22]
// UNKNOWN: whether item replenish has the 500 pass cap that creature restock has. [c9, c22]
bool rollStoreReplenish(Ctx& ctx, i32 daysAged) {
  return ctx.rng.percent(storeReplenishChance(daysAged));
}

// Starting store alignment test: numeric test `AlignmentFlags > 1`. [c22]
// CONFLICT: [c34] describes a bit-count test (more than one bit set). Preferred: engine [c22].
bool storeOffersAlignmentChoice(i32 alignmentFlags) {
  return alignmentFlags > 1;
}

// Companion buy price from base value and pen count. [c9]
//   value  = max(BaseValue, 10)
//   stock  = min(penned(align) - 1, 9)
//   price  = (1.5 * value) / (ln(stock + 1) + 1.15)
// CONFLICT: [c55] gives round_half_even(1.5 * BaseValue / 1.15) with no floor and no ln term.
// Preferred: engine formula [c9].
// UNKNOWN: penned == 0 price. The engine form is undefined there (ln(0)); [c55] gives 0.
// Returns 0 when nothing is penned.
// UNKNOWN: rounding of the result to whole gold; store price rounding is unverified [c34].
Currency companionBuyPrice(i64 baseValue, i32 penned) {
  if (penned <= 0) {
    return 0;
  }
  const i64 value = baseValue > kCompanionPriceFloor ? baseValue : kCompanionPriceFloor;
  const i32 penStock = penned - 1;
  const i32 stock = penStock < kCompanionStockCap ? penStock : kCompanionStockCap;
  const double price = (1.5 * static_cast<double>(value)) /
                       (std::log(static_cast<double>(stock) + 1.0) + 1.15);
  return currencyFromWhole(roundHalfEven(price));
}

// Sighting Known update: Known is only ever raised. [c12]
// CONFLICT: [c57] says "set Known to the grade". Preferred: engine, raise only [c12].
i32 mergeSightingKnown(i32 knownGrade, i32 readGrade) {
  return readGrade > knownGrade ? readGrade : knownGrade;
}

// Monster merge name: the newer name replaces the old one when there.Known == 0. [c12]
// CONFLICT: [c57] says the first recorder's name is kept. Preferred: engine [c12].
bool sightingTakesNewName(i32 existingKnown) {
  return existingKnown == 0;
}

// Invisible sighting rule: an invisible creature is skipped only when the reader is in the
// dungeon (DungeonLevel > 0) and lacks See Invisible. [c12]
// CONFLICT: [c57] skips with no level condition. Preferred: engine [c12].
bool sightingSkipsInvisible(i32 readerDungeonLevel, bool creatureInvisible, bool readerSeesInvisible) {
  return creatureInvisible && !readerSeesInvisible && readerDungeonLevel > 0;
}

}  // namespace mordor
