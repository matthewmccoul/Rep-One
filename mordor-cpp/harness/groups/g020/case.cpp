#include "case_api.h"
#include "mordor/state_character.h"
#include "mordor/attributes.h"

namespace mordor {

// status2.cpp copy is not file-local: harness renames it to paralyse__status2.
void paralyse__status2(Ctx& ctx, Character& c, i32 level);

// status1a.cpp copy is file-local and same-name: reached through shim_status1a.cpp.
void paralyse_via_status1a(Ctx& ctx, Character& c, i32 level);

// Same construction as the C# side: Human, Nomad starting guild, same six stats (STR,INT,WIS,CON,CHA,DEX).
static void build(const JsonValue& a, Ctx& ctx, Character& c) {
    createCharacter(ctx, c, Race::Human, Guild::Nomad,
        {static_cast<i32>(A(a, "stat_str").asInt()),
         static_cast<i32>(A(a, "stat_int").asInt()),
         static_cast<i32>(A(a, "stat_wis").asInt()),
         static_cast<i32>(A(a, "stat_con").asInt()),
         static_cast<i32>(A(a, "stat_cha").asInt()),
         static_cast<i32>(A(a, "stat_dex").asInt())});
}

// Case-specific state set after construction, as on the C# side.
static void setup(const JsonValue& a, Character& c) {
    c.ailments.paralysisLevel = static_cast<i32>(A(a, "prior_level").asInt());
    c.combat.adShift = static_cast<i32>(A(a, "prior_adshift").asInt());
}

static Str canon(const Character& c) {
    return I(c.ailments.paralysisLevel) + "|" + I(c.combat.adShift);
}

static Str run_status1a(const JsonValue& a, Ctx& ctx) {
    Character c;
    build(a, ctx, c);
    setup(a, c);
    paralyse_via_status1a(ctx, c, static_cast<i32>(A(a, "level").asInt()));
    return canon(c);
}

static Str run_status2(const JsonValue& a, Ctx& ctx) {
    Character c;
    build(a, ctx, c);
    setup(a, c);
    paralyse__status2(ctx, c, static_cast<i32>(A(a, "level").asInt()));
    return canon(c);
}

const CopyEntry kCopies[] = {
    {"paralyse@status1a", &run_status1a},
    {"paralyse@status2", &run_status2},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor

