#include "test_support.h"
#include "mordor/spells1b.h"
#include "mordor/state_party.h"

namespace {

struct FateStats {
    mordor::i32 hitsMin = 1000;
    mordor::i32 hitsMax = -1000;
    double hitsSum = 0.0;
    mordor::i32 spMin = 1000;
    mordor::i32 spMax = -1000;
    double spSum = 0.0;
    mordor::i32 draws = 0;
};

}  // namespace

// B_0063#18-#21: SKIP. applyCastState takes a numeric effect id, and the
// mapping from the MDATA2.MDR effect names (Levitate, Invisible, SeeInvisible,
// Protect) to those ids is not given in the headers, so the inputs cannot be
// set up faithfully.
// SKIP B_0063#18: effect-name to effect-id mapping not in headers; applyCastState is id-based
// SKIP B_0063#19: effect-name to effect-id mapping not in headers; applyCastState is id-based
// SKIP B_0063#20: effect-name to effect-id mapping not in headers; applyCastState is id-based
// SKIP B_0063#21: effect-name to effect-id mapping not in headers; applyCastState is id-based

