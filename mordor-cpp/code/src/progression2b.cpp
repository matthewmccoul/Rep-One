#include "mordor/progression2b.h"

#include <algorithm>
#include <cmath>

#include "mordor/math_util.h"

namespace mordor {
namespace {

// kCurrencyScale (i64, 1/10000 gold) comes from types.h (T7); the local duplicate
// was removed because it made unqualified references ambiguous.

// T10: hits cap 999, MaxHits = min(MaxHits, 999) [c2].
constexpr i32 kHitsCap = 999;

// T7 / gold closed form: base 30.1, guild gold factor term (1 + ggf/10) [c104].
constexpr double kGoldBase = 30.1;
constexpr double kGoldFactorDivisor = 10.0;

// T7 / T10 / test anchors: Human Nomad XP threshold polynomial in n = target level.
// Coefficients fitted to anchors 378,045 (n=66), 556,907 (n=80), 872,326 (n=100);
// constants 88.09020776 and 45.45454 are the T10 XP_Total constants [c104].
constexpr double kXpQuadratic = 88.09020776;
constexpr double kXpLinear = -85.31;
constexpr double kXpConstant = -45.45454;

// T9: rest age cost B((100 - 100*cur/max)/3.25) [c24][c25].
constexpr double kRestAgeDivisor = 3.25;

// T9: poison clear result > 185, cost 2 HP [c23].
constexpr i32 kPoisonClearAbove = 185;
constexpr i32 kPoisonCureHitCost = 2;

// T9: disease clear result > 195, cost 1 HP [c23].
constexpr i32 kDiseaseClearAbove = 195;
constexpr i32 kDiseaseCureHitCost = 1;

// T9: old-age onset at 98% of racial lifespan [c23][c25][c97].
constexpr i64 kOldAgePercent = 98;

// Natural death chance: 5% + Log(excess) [c25].
constexpr double kNaturalDeathBase = 5.0;

// Sanctuary ages 365 days in the original [c31][c34].
constexpr i32 kSanctuaryAgeDays = 365;

// T11: bonus HP while Con >= 17 for each level below ML [c106].
constexpr i32 kBonusHitsConMin = 17;
// UNKNOWN: amount of the bonus HP per level is not stated in the SRD; 0 is used until confirmed.
constexpr i32 kBonusHitsPerLevel = 0;

// T11: Nomad levels 29-30 give 5 HP more [c106].
constexpr i32 kNomadExtraLevelLo = 29;
constexpr i32 kNomadExtraLevelHi = 30;
constexpr i32 kNomadExtraHits = 5;

// T11: past ML, Nomad gives 1 HP per level, other guilds 2 HP per level [c106].
constexpr i32 kNomadPastMaxHits = 1;
constexpr i32 kOtherPastMaxHits = 2;

}  // namespace


}  // namespace mordor
