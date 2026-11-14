#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct CharStatForm {
    static constexpr int32_t Resource = 8;

    static constexpr int32_t FrameLeft = 195 - (30 + 180);
    static constexpr int32_t FrameTop = 645 - (30 + 510);

    inline static FormRect elaTabFrame0 = FormRect{195, 645, 3540, 4350};   // on the form
    inline static FormRect lblRaceInfo = FormRect{45, 60, 3450, 215};
    inline static FormRect lblGuildInfo = FormRect{45, 300, 3450, 215};
    inline static FormRect elaGroupInfo = FormRect{60, 570, 3450, 225};
    inline static FormRect elaBorder = FormRect{75, 1770, 3405, 120};
    inline static FormRect lblStatInfo0 = FormRect{270, 885, 1530, 215};
    inline static FormRect lblStat0 = FormRect{1965, 885, 1455, 215};
    inline static FormRect lblSpecial0 = FormRect{2205, 2260, 1260, 215};

    inline static cs::List<cs::String> Captions = cs::List<cs::String>::Of({"Age", "Spells", "Spell Level", "Hits", "Att/Def", "Experience", "Gold"});

    static constexpr int32_t StatFigureWidth = 400;

    static int32_t RowTop(int32_t n);

    static Tup_Left_Width_627c0 Figure(int32_t n);

    static int32_t BorderTop();

    static int32_t SpecialTop(int32_t n);

    inline static cs::List<ServiceForms_Control> Controls = cs::List<ServiceForms_Control>::Of({
        ServiceForms_Control{"elaTabFrame", 0, elaTabFrame0}, ServiceForms_Control{"lblRaceInfo", -1, lblRaceInfo}, ServiceForms_Control{"lblGuildInfo", -1, lblGuildInfo},
        ServiceForms_Control{"elaGroupInfo", -1, elaGroupInfo}, ServiceForms_Control{"elaBorder", -1, elaBorder}, ServiceForms_Control{"lblStatInfo", 0, lblStatInfo0},
        ServiceForms_Control{"lblStat", 0, lblStat0}, ServiceForms_Control{"lblSpecial", 0, lblSpecial0},
    });
};

}  // namespace mc
