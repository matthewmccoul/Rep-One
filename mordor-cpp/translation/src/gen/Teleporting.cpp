#include "core.h"

namespace mc {

Teleporting_Outcome Teleporting::ForSpell(cs::List<cs::Ref<Floor>> dungeon, int32_t level, int32_t x, int32_t y) {
    if (level <= 0) return Outcome::FallsFromTheSky;
    // past mc0276 counts as rock whatever is there (5860); the shipped dungeon has exactly
    // mc0276 levels (selfcheck), so every level up to it has a map to test
    if (level > SpellEffect::MaxDungeonLevel || level > dungeon.Count()) return Outcome::IntoRock;
    auto fl = dungeon[level - 1];
    if (x < 0 || y < 0 || x >= fl->Width || y >= fl->Height) return Outcome::IntoRock;
    return fl->Has(x, y, Floor::ROCK) ? Outcome::IntoRock : Outcome::Arrives;
}

cs::List<std::tuple<int32_t, int32_t, int32_t>> Teleporting::ForSquare(cs::List<cs::Ref<Floor>> dungeon, int32_t fromLevel,
                                                                       Floor_Teleporter t, int32_t members, cs::Ref<cs::Random> rng) {
    auto dest = cs::List<std::tuple<int32_t, int32_t, int32_t>>::New();
    int32_t loaded = fromLevel;
    for (int32_t i = 0; i < members; i++) {
        int32_t x, y;
        if (t.X2 == 0 || t.Y2 == 0) {
            auto fl = dungeon[loaded - 1];
            do { x = rng->Next(fl->Width); y = rng->Next(fl->Height); }
            while (fl->Has(x, y, Floor::ROCK) || fl->Has(x, y, Floor::TELEPORTER));
        } else { x = t.X2 - 1; y = t.Y2 - 1; }
        int32_t level = t.Z2 > 0 && t.Z2 != fromLevel ? (int32_t)t.Z2 : fromLevel;
        dest.Add(std::make_tuple(level, x, y));
        if (std::get<0>(dest[0]) >= 1 && std::get<0>(dest[0]) <= dungeon.Count()) loaded = std::get<0>(dest[0]);
    }
    return dest;
}

}  // namespace mc
