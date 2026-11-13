#include "case_api.h"
#include "mordor/progression1a.h"
#include "mordor/progression2.h"
#include "mordor/progression2b.h"
#include "mordor/state_character.h"

namespace mordor {

// Same-name copies, renamed by the harness; declared here with the original signatures.
i64 experienceCap__progression1a(const Ctx& ctx, const Character& ch, Guild g);
i64 experienceCap__progression2b(i32 level, double guildExpFactor, double raceExpFactor);

// Factor getters exported from progression1a.cpp by shim_progression1a.cpp.
double guildExpFactor_factor__progression1a(const Ctx& ctx, Guild g);
double raceExpFactor_factor__progression1a(const Ctx& ctx, Race r);

static Character nomadAt(i32 level) {
    Character ch;
    ch.race = Race::Human;
    ch.guilds[static_cast<u32>(Guild::Nomad)].joined = true;
    ch.guilds[static_cast<u32>(Guild::Nomad)].level = level;
    return ch;
}

static Str run_experienceCapOf(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const Character ch = nomadAt(static_cast<i32>(A(a, "level").asInt()));
    return I(experienceCapOf(ch, Guild::Nomad));
}

static Str run_experienceCap_progression1a(const JsonValue& a, Ctx& ctx) {
    const Character ch = nomadAt(static_cast<i32>(A(a, "level").asInt()));
    return I(experienceCap__progression1a(ctx, ch, Guild::Nomad));
}

// Factors come from the C++ tables through ctx, via the shim exports, the same Nomad and Human values the reference uses.
static Str run_experienceCap_progression2b(const JsonValue& a, Ctx& ctx) {
    const Character ch = nomadAt(static_cast<i32>(A(a, "level").asInt()));
    const i32 level = ch.guilds[static_cast<u32>(Guild::Nomad)].level;
    const double gf = guildExpFactor_factor__progression1a(ctx, Guild::Nomad);
    const double rf = raceExpFactor_factor__progression1a(ctx, ch.race);
    return I(experienceCap__progression2b(level, gf, rf));
}

const CopyEntry kCopies[] = {
    {"experienceCapOf", &run_experienceCapOf},
    {"experienceCap@progression1a", &run_experienceCap_progression1a},
    {"experienceCap@progression2b", &run_experienceCap_progression2b},
};
const std::size_t kNumCopies = 3;

}  // namespace mordor
