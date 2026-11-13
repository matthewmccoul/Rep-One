#include "test_support.h"

#include "mordor/state_character.h"
#include "mordor/status2.h"

using namespace mordor;

// SKIP B_0066#121: stat part (StatsADNow + StoredADShift) is not computable from the headers; no function derives it from stats and items.
// SKIP B_0066#122: stat part after paralysis is not queryable; no accessor combines StatsADNow and StoredADShift.
// SKIP B_0066#125: whether a level-0 monster paralyses depends on a roll/gating rule not specified in the headers (tryParalysis outcome is RNG and chance dependent).
// SKIP B_0066#126: stat part after a second paralysis is not queryable; no function derives the combined value.
// SKIP B_0066#127: stat part and ADShift after a Str raise are not queryable; no function derives the stat part and stat raises are not in the API.
// SKIP B_0066#128: save and reload is not exposed in the headers; no serialisation function exists.
// SKIP B_0066#129: save and reload is not exposed in the headers; no serialisation function exists.
// SKIP B_0066#130: save and reload is not exposed in the headers; stat part is not queryable.

// Partial check of B_0066#123: the Paralysed component after Unparalyse is False.
// The stat part (32) and ADShift (0) are not queryable, so only the Paralysed component is asserted.
TEST(engine_B_0066_123_unparalyse_clears_paralysed) {
    tsup::Env env({0.5, 0.9});
    Character c{};
    paralyse(env.ctx, c, 3);
    unparalyse(env.ctx, c);
    CHECK_EQ(c.ailments.paralysed, false);
}

// Partial check of B_0066#124: the Paralysed component after the city gate is False.
// The stat part (0) and ADShift (-32) are not queryable, so only the Paralysed component is asserted.
TEST(engine_B_0066_124_city_gate_clears_paralysed) {
    tsup::Env env({0.5, 0.9});
    Character c{};
    paralyse(env.ctx, c, 3);
    clearParalysisAtCityGate(env.ctx, c);
    CHECK_EQ(c.ailments.paralysed, false);
}
