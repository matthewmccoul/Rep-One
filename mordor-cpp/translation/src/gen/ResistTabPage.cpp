#include "core.h"

namespace mc {

cs::List<CharStatFile_Piece> ResistTabPage::Paint(cs::Ref<Character> c, cs::Func<cs::Ref<Item>(int16_t)> item) {
    auto p = cs::List<CharStatFile_Piece>::Of({CharStatFile_Piece{2, 1, "Current Resistances", true}});
    for (int32_t i = 0; i < Effects::ResistanceKinds; i++) {
        int32_t pct = Effects::ResistancePercent(c, i, item);
        p.Add(CharStatFile_Piece{4 + i, 2, Librarian::Resistances[i], false});
        p.Add(CharStatFile_Piece{4 + i, 12, pct > 0 ? cs::ToStr(pct) + "%" : cs::String(" -"), false});
    }
    return p;
}

}  // namespace mc
