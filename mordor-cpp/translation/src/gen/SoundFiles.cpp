#include "core.h"

namespace mc {

cs::String SoundFiles::Name(int32_t n) { return Names[n]; }

cs::List<int32_t> SpellSounds::Numbers(int32_t kind, int32_t level) {
    int32_t first;
    if (kind == 0) first = Fire;
    else if (kind == 2) first = Shock;
    else if (kind == 1) first = Cold;
    else if (kind == 9) first = Magic2;
    else first = Magic;
    return (kind == 0 || kind == 2 || kind == 1) && level >= 7 ? cs::List<int32_t>::Of({first, Explos}) : cs::List<int32_t>::Of({first});
}

cs::List<cs::String> SpellSounds::For(cs::Ref<Spell> s) {
    return Numbers(cs::deref(s).Category, s->Level).Select([&](auto& n) { return SoundFiles::Name(n); }).ToArray();
}

}  // namespace mc
