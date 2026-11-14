#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct BarColours {
    inline static Tup_R_G_B_5bdd2 Track = Tup_R_G_B_5bdd2{0x2a, 0x2a, 0x38};

    // the hits bar's three stops: full is green, half is yellow, empty is red
    inline static Tup_R_G_B_5bdd2 HitsFull = Tup_R_G_B_5bdd2{60, 200, 70};
    inline static Tup_R_G_B_5bdd2 HitsHalf = Tup_R_G_B_5bdd2{230, 210, 50};
    inline static Tup_R_G_B_5bdd2 HitsEmpty = Tup_R_G_B_5bdd2{220, 50, 40};

    // the spell points bar's: full is a bright blue, half a violet, empty a dim purple
    inline static Tup_R_G_B_5bdd2 SpFull = Tup_R_G_B_5bdd2{70, 150, 255};
    inline static Tup_R_G_B_5bdd2 SpHalf = Tup_R_G_B_5bdd2{150, 95, 235};
    inline static Tup_R_G_B_5bdd2 SpEmpty = Tup_R_G_B_5bdd2{120, 60, 150};

    inline static Tup_R_G_B_5bdd2 ToNext = Tup_R_G_B_5bdd2{50, 175, 95};

    inline static Tup_R_G_B_5bdd2 ToPin = Tup_R_G_B_5bdd2{205, 70, 185};

    static constexpr uint8_t UnderTextAlpha = 0x80;

    inline static Tup_R_G_B_5bdd2 TextOver = Tup_R_G_B_5bdd2{0xf5, 0xf5, 0xf5};
};

}  // namespace mc
