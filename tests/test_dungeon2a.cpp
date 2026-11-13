#include "test_support.h"
#include "mordor/dungeon2a.h"

using namespace mordor;

// R137 / automap: Square = X + 30*(Y-1), with X and Y starting at 1.
TEST(squareIndexOf_rowMajorFromOne) {
    CHECK_EQ(squareIndexOf(1, 1), 1);
    CHECK_EQ(squareIndexOf(30, 1), 30);
    CHECK_EQ(squareIndexOf(1, 2), 31);
    CHECK_EQ(squareIndexOf(30, 30), 900);
    CHECK_EQ(kFloorWidth, 30);
}

