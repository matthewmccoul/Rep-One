#include "test_support.h"
#include "mordor/dataformatsb.h"

using namespace mordor;

// ---- option and port flag text parsing (SRD c10) ----

// ---- record addressing (SRD c17, c35) ----

// ---- fixed-width little-endian reads (SRD c17, c91) ----

// ---- writers and string fields (SRD c17, c91) ----

// ---- raw record layouts (SRD c13) ----

// ---- table and pack addressing (SRD c13, c62) ----

// ---- vital and regenerated data files (SRD c96) ----

// ---- automap flag bit (SRD c35) ----

// ---- IdLevel saved codes (SRD c35) ----

// ---- guild log MDATA9 (SRD c10) ----

// ---- mod settings escaping and encoding (SRD c41) ----

// ---- mod flags, has-key and value normalisation (SRD c41) ----

// ---- mod save tags (SRD c40) ----

// ---- store key and legacy INI (SRD c22, c34) ----

// ---- name collision and save refusal (SRD c3, c53) ----

// ---- active guild switching (SRD c35) ----

// ---- enum values (SRD c35) ----

TEST(AlignAndSexEnumValues) {
    CHECK(static_cast<i32>(Align::Good) == 0);
    CHECK(static_cast<i32>(Align::Neutral) == 1);
    CHECK(static_cast<i32>(Align::Evil) == 2);
    CHECK(static_cast<i32>(Sex::Male) == 0);
    CHECK(static_cast<i32>(Sex::Female) == 1);
}
