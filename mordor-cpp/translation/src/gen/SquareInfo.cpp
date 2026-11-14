#include "core.h"

namespace mc {

cs::String SquareInfo::FellIntoPit(int32_t hits) {
    return "You have fallen into a pit for " + cs::ToStr(hits) + " hits.";
}

cs::String SquareInfo::TheyFellIntoPit(cs::String who, int32_t hits) {
    return "\"" + cs::Str::TrimEnd(who) + "\" has fallen into a pit for " + cs::ToStr(hits) + " hits.";
}

cs::String SquareInfo::Of(cs::Ref<Floor> fl, int32_t x, int32_t y) {
    cs::String said = "";
    if (fl->Has(x, y, Floor::STAIRS_UP)) said = StairsUp;
    if (fl->Has(x, y, Floor::STAIRS_DOWN)) said = StairsDown;
    if (fl->Has(x, y, Floor::CHUTE)) said = SquareInfo::Chute;
    return said;
}

}  // namespace mc
