#include "case_api.h"
#include "mordor/casting1.h"
#include "mordor/casting2.h"

namespace mordor {

// Pure function: ctx.rng is not consumed.
static Str run_guildSpellLevel(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    i32 guildLevel = static_cast<i32>(A(a, "guildLevel").asInt());
    return I(guildSpellLevel(guildLevel));
}

static Str run_spellLevelFromGuildLevel(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    i32 guildLevel = static_cast<i32>(A(a, "guildLevel").asInt());
    return I(spellLevelFromGuildLevel(guildLevel));
}

const CopyEntry kCopies[] = {
    {"guildSpellLevel", &run_guildSpellLevel},
    {"spellLevelFromGuildLevel", &run_spellLevelFromGuildLevel},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
