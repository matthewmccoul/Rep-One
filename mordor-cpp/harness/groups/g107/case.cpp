#include "case_api.h"
#include "mordor/combat1a.h"
#include "mordor/companions1b.h"
#include "mordor/state_creature.h"

namespace mordor {

static Str run_monsterHasBit(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const long long flags = A(a, "flags").asInt();
    const i32 bit = static_cast<i32>(A(a, "bit").asInt());
    Creature c;
    c.specialPropertyFlags = static_cast<decltype(c.specialPropertyFlags)>(static_cast<u32>(flags));
    return monsterHasBit(c, bit) ? "1" : "0";
}

static Str run_specialFlagBit(const JsonValue& a, Ctx& ctx) {
    (void)ctx;
    const u32 flags = static_cast<u32>(A(a, "flags").asInt());
    const i32 bit = static_cast<i32>(A(a, "bit").asInt());
    return specialFlagBit(flags, bit) ? "1" : "0";
}

const CopyEntry kCopies[] = {
    {"monsterHasBit", &run_monsterHasBit},
    {"specialFlagBit", &run_specialFlagBit},
};
const std::size_t kNumCopies = 2;

}
