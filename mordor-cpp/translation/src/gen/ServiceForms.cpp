#include "core.h"

namespace mc {

cs::String FormRect::ToString() {
    return cs::ToStr(Left) + "," + cs::ToStr(Top) + " " + cs::ToStr(Width) + "x" + cs::ToStr(Height);
}

FormRect ServiceForms_Store::ListArea() {
    return FormRect{elaItem0.Left, ItemHead.Top,
                    vsbItems.Left + vsbItems.Width - elaItem0.Left, elaItem0.Top + 9 * elaItem0.Height - ItemHead.Top};
}

bool ServiceForms_Companions::RightHand(int32_t slot) {
    return (slot & 1) == 1;
}

}  // namespace mc
