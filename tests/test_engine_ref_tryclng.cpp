#include "test_support.h"
#include "mordor/ref_tryclng.h"

using namespace mordor;

// B_0062#71: surrounding spaces are trimmed.
TEST(engine_ref_tryclng_padded_spaces) {
    i32 v = -999;
    CHECK(tryClng(" 250 ", v));
    CHECK_EQ(v, 250);
}

// B_0062#72: &H prefix is hexadecimal.
TEST(engine_ref_tryclng_ampersand_hex) {
    i32 v = -999;
    CHECK(tryClng("&H10", v));
    CHECK_EQ(v, 16);
}

// B_0062#73: &O prefix is octal.
TEST(engine_ref_tryclng_ampersand_octal) {
    i32 v = -999;
    CHECK(tryClng("&O17", v));
    CHECK_EQ(v, 15);
}

// B_0062#74: &HFFFF wraps to -1 through the 16-bit (short) path.
TEST(engine_ref_tryclng_hex_wraps_to_short) {
    i32 v = -999;
    CHECK(tryClng("&HFFFF", v));
    CHECK_EQ(v, -1);
}

// B_0062#75: E exponent.
TEST(engine_ref_tryclng_exponent_e) {
    i32 v = -999;
    CHECK(tryClng("1E3", v));
    CHECK_EQ(v, 1000);
}

// B_0062#76: D exponent is the double-precision spelling of E.
TEST(engine_ref_tryclng_exponent_d) {
    i32 v = -999;
    CHECK(tryClng("2.5D2", v));
    CHECK_EQ(v, 250);
}

// B_0062#77: round half to even, down.
TEST(engine_ref_tryclng_round_half_even_down) {
    i32 v = -999;
    CHECK(tryClng("2.5", v));
    CHECK_EQ(v, 2);
}

// B_0062#78: round half to even, up.
TEST(engine_ref_tryclng_round_half_even_up) {
    i32 v = -999;
    CHECK(tryClng("3.5", v));
    CHECK_EQ(v, 4);
}

// B_0062#79: negative decimal integer.
TEST(engine_ref_tryclng_negative_integer) {
    i32 v = -999;
    CHECK(tryClng("-7", v));
    CHECK_EQ(v, -7);
}

// B_0062#80: non-numeric text is refused. The reference's test records int.MinValue
// as the refusal sentinel; the function itself leaves value at 0, which is checked here.
TEST(engine_ref_tryclng_refuses_non_number) {
    i32 v = -999;
    CHECK(!tryClng("abc", v));
    CHECK_EQ(v, 0);
}

// B_0062#81: a value past the Long range is refused. Same sentinel note as #80.
TEST(engine_ref_tryclng_refuses_past_long_range) {
    i32 v = -999;
    CHECK(!tryClng("3000000000", v));
    CHECK_EQ(v, 0);
}
