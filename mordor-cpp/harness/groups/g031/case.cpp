#include "case_api.h"
#include "mordor/state_character.h"
#include "mordor/progression2.h"

namespace mordor {

// Renamed same-name copy from src/progression2.cpp.
i32 restInCity__progression2(Ctx& ctx, Character& ch);

// Export wrapper for the file-local copy in src/progression1b.cpp (see shim_progression1b.cpp).
i32 shimRestInCity1b(Ctx& ctx, Character& ch, bool partyMany);

static Str combine(i32 days, const Character& ch) {
    Str s = I(days);
    s += "|";
    s += I(ch.ageDays);
    return s;
}

static Str run_restInCity_progression2(const JsonValue& a, Ctx& ctx) {
    Character ch;
    ch.vitals.curHits = static_cast<i32>(A(a, "curHits").asInt());
    ch.vitals.maxHits = static_cast<i32>(A(a, "maxHits").asInt());
    ch.ageDays = static_cast<i32>(A(a, "age").asInt());
    const i32 days = restInCity__progression2(ctx, ch);
    return combine(days, ch);
}

static Str run_restInCity_progression1b(const JsonValue& a, Ctx& ctx) {
    Character ch;
    ch.vitals.curHits = static_cast<i32>(A(a, "curHits").asInt());
    ch.vitals.maxHits = static_cast<i32>(A(a, "maxHits").asInt());
    ch.ageDays = static_cast<i32>(A(a, "age").asInt());
    const bool partyMany = A(a, "partyMany").asBool();
    const i32 days = shimRestInCity1b(ctx, ch, partyMany);
    return combine(days, ch);
}

const CopyEntry kCopies[] = {
    {"restInCity@progression2", &run_restInCity_progression2},
    {"restInCity@progression1b", &run_restInCity_progression1b},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
