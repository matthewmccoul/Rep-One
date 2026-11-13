#include "test_support.h"
#include "mordor/city1.h"

// SKIP B_0059#49: the expected count depends on a 150-draw script whose exact values are not given, so the scripted restock cannot be reproduced.
// SKIP B_0059#50: no draw-count accessor exists on IRng or ScriptedRng in the headers, so the number of draws cannot be observed.
// SKIP B_0059#51: no draw-count accessor exists, and the 100-empty-kind script is not specified exactly enough to reproduce.
// SKIP B_0059#95: checks the guild form caption in FRMCTYG.BAS, which needs UI and the original source, not an engine function.
// SKIP B_0061#162: needs MdrLoader.LoadItems and MDATA3, which are not in the headers.
// SKIP B_0063#39: no uncurse-check function (Uncursing.Refusal) exists in the headers.
// SKIP B_0063#40: no uncurse-check function (Uncursing.Refusal) exists in the headers.
// SKIP B_0063#41: no uncurse-check function (Uncursing.Refusal) exists in the headers.
// SKIP B_0063#42: no uncurse-check function (Uncursing.Refusal) exists in the headers.

// B_0063#1: restockPens draws monsters from tsup::data(), and the per-monster filter is applied internally.
TEST(engine_B_0063_1_confinement_replenish_peak_count) {
    tsup::Env env(4);
    mordor::PenBook pens{};
    for (mordor::i32 call = 0; call < 4000; ++call) {
        mordor::restockPens(env.ctx, pens, 50, 14);
    }
    mordor::i32 peak = 0;
    for (auto r = 0u; r < pens.byRef.size(); ++r) {
        for (auto a = 0u; a < static_cast<unsigned>(mordor::kPenAlignCount); ++a) {
            if (pens.byRef[r][a] > peak) {
                peak = pens.byRef[r][a];
            }
        }
    }
    CHECK(peak >= mordor::kPenShelfCap + 1);
}
