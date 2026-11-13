#include "case_api.h"
#include "mordor/items1a.h"
#include "mordor/items1b.h"

namespace mordor {

static Str run_isMustBeEquippedType(const JsonValue& a, Ctx& ctx) {
    (void)ctx;  // rule does not draw random numbers
    i32 t = static_cast<i32>(A(a, "type").asInt());
    return I(isMustBeEquippedType(t) ? 1 : 0);
}

static Str run_mustBeEquipped(const JsonValue& a, Ctx& ctx) {
    (void)ctx;  // rule does not draw random numbers
    i32 t = static_cast<i32>(A(a, "type").asInt());
    return I(mustBeEquipped(t) ? 1 : 0);
}

const CopyEntry kCopies[] = {
    {"isMustBeEquippedType", &run_isMustBeEquippedType},
    {"mustBeEquipped", &run_mustBeEquipped},
};
const std::size_t kNumCopies = 2;

}  // namespace mordor
