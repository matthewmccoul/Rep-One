#include "case_api.h"
#include "mordor/state_character.h"

namespace mordor {

// Same-name copies, renamed by the harness (signatures match the originals).
double oldAgePercent__death1b(i32 ageDays, i32 raceLifespanYears);
double oldAgePercent__death2b(const Character& ch, i32 raceLifespanYears);

static Str run_oldAgePercent_death1b(const JsonValue& a, Ctx& ctx) {
    (void)ctx;  // no rng draws consumed by this rule
    const i32 ageDays = static_cast<i32>(A(a, "ageDays").asInt());
    const i32 lifespan = static_cast<i32>(A(a, "raceLifespanYears").asInt());
    return F(oldAgePercent__death1b(ageDays, lifespan));
}

static Str run_oldAgePercent_death2b(const JsonValue& a, Ctx& ctx) {
    (void)ctx;  // no rng draws consumed by this rule
    const i32 ageDays = static_cast<i32>(A(a, "ageDays").asInt());
    const i32 lifespan = static_cast<i32>(A(a, "raceLifespanYears").asInt());
    Character ch{};
    ch.ageDays = ageDays;
    return F(oldAgePercent__death2b(ch, lifespan));
}

const CopyEntry kCopies[] = {
    {"oldAgePercent@death1b", &run_oldAgePercent_death1b},
    {"oldAgePercent@death2b", &run_oldAgePercent_death2b},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
