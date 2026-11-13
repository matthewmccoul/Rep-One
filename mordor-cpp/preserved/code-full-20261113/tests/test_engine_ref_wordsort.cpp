#include "test_support.h"
#include "mordor/ref_wordsort.h"

using namespace mordor;

// B_0058#28: Archer / Arch-Fiend / Archmage group order.
TEST(engine_ref_wordsort_archer_group) {
    CHECK(wordSort("Archer", "Arch-Fiend") < 0);
    CHECK(wordSort("Arch-Fiend", "Archmage") < 0);
    CHECK(wordSort("Archer", "Archmage") < 0);
    CHECK(wordSort("Archmage", "Archer") > 0);
}

// B_0058#28: Dragonfly / Dragons / Dragon's Blood group order.
TEST(engine_ref_wordsort_dragon_group) {
    CHECK(wordSort("Dragonfly", "Dragons") < 0);
    CHECK(wordSort("Dragons", "Dragon's Blood") < 0);
    CHECK(wordSort("Dragonfly", "Dragon's Blood") < 0);
    CHECK(wordSort("Dragon's Blood", "Dragonfly") > 0);
}

// B_0058#28: Tarantula / T'Kovant group order.
TEST(engine_ref_wordsort_tarantula_group) {
    CHECK(wordSort("Tarantula", "T'Kovant") < 0);
    CHECK(wordSort("T'Kovant", "Tarantula") > 0);
}

// Names equal under the word sort are told apart by the characters left out (reference doc comment).
TEST(engine_ref_wordsort_tie_by_marks) {
    CHECK(wordSort("Arch-Fiend", "ArchFiend") < 0);
    CHECK(wordSort("ArchFiend", "Arch-Fiend") > 0);
}

// Names that differ in case alone compare equal (reference doc comment).
TEST(engine_ref_wordsort_case_only_equal) {
    CHECK_EQ(wordSort("Archer", "archer"), 0);
    CHECK_EQ(wordSort("DRAGONS", "dragons"), 0);
}
