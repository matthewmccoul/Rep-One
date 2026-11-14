#include "core.h"

namespace mc {

int32_t CharStatForm::RowTop(int32_t n) {
    int32_t half = cs::CastDouble<int32_t>(cs::Math::Round(lblStat0.Height / 2.0, cs::MidpointRounding::ToEven));
    int32_t top = lblStat0.Top;
    for (int32_t i = 1; i <= n; i++)
        top += lblStat0.Height + ((i == 1 || i == 3) ? half : i == 7 ? half * 2 : 0);
    return top;
}

Tup_Left_Width_627c0 CharStatForm::Figure(int32_t n) {
    return n < 7
        ? Tup_Left_Width_627c0{lblStat0.Left, lblStat0.Width}
        : Tup_Left_Width_627c0{lblStatInfo0.Left + lblStatInfo0.Width + 1, StatFigureWidth};
}

int32_t CharStatForm::BorderTop() {
    return cs::CastDouble<int32_t>(cs::Math::Round(RowTop(7) - lblStat0.Height / 1.2, cs::MidpointRounding::ToEven));
}

int32_t CharStatForm::SpecialTop(int32_t n) {
    return RowTop(7) + n * lblSpecial0.Height;
}

}  // namespace mc
