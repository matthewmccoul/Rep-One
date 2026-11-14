#include "core.h"

namespace mc {

cs::String GuildTitles::Of(cs::String guild, int32_t level) {
    cs::List<Tup_Below_Title_e8866> bands{};
    if (!Bands.TryGetValue(guild, bands) || bands.Length() == 0) return "Adventurer";
    for (auto& _t : bands) {
        auto below = _t.Below;
        auto title = _t.Title;
        if (level < below) return title;
    }
    return bands[bands.Length() - 1].Title;
}

}  // namespace mc
