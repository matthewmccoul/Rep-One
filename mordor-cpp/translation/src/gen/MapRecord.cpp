#include "core.h"

namespace mc {

cs::List<int32_t> MapRecord::OwnEdges =
    cs::List<int32_t>::Of({Floor::WALL_EAST, Floor::WALL_NORTH, Floor::DOOR_EAST, Floor::DOOR_NORTH});

cs::List<int32_t> MapRecord::SecretEdges = cs::List<int32_t>::Of({Floor::SECRET_DOOR_EAST, Floor::SECRET_DOOR_NORTH});

cs::List<int32_t> MapRecord::Features =
    cs::List<int32_t>::Of({Floor::PIT, Floor::STAIRS_UP, Floor::STAIRS_DOWN, Floor::WATER, Floor::QUICKSAND, Floor::FOG, Floor::CHUTE});

int32_t MapRecord::WallBehind(int32_t secretBit) {
    return secretBit == Floor::SECRET_DOOR_EAST ? Floor::WALL_EAST : Floor::WALL_NORTH;
}

bool MapRecord::NoticesSecretDoor(cs::Ref<Character> c, cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    return rng->Next(100) < Rules::BestAbilityValue(c, 5, gear) / 5;
}

void MapRecord::OnEntry(cs::Ref<SharedMap> map, cs::Ref<Floor> fl, cs::Ref<Character> c, int32_t x, int32_t y,
                        cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    if (c->LostPosition || c->LostLevel) return;
    int32_t level = fl->LevelNum, w = fl->Width;

    // Fog is read once from your own square (MAZEMOD.BAS:1570, 1646, 1671), not the neighbour's.
    bool fogHere = fl->Has(x, y, Floor::FOG);

    Square(map, fl, c, level, w, x, y, rng, gear, fogHere, true);

    // the east side of the square to the west, and the north side of the one to the south
    if (x > 0) Square(map, fl, c, level, w, x - 1, y, rng, gear, fogHere, false,
                      cs::List<int32_t>::Of({Floor::WALL_EAST, Floor::DOOR_EAST}), Floor::SECRET_DOOR_EAST);
    if (y > 0) Square(map, fl, c, level, w, x, y - 1, rng, gear, fogHere, false,
                      cs::List<int32_t>::Of({Floor::WALL_NORTH, Floor::DOOR_NORTH}), Floor::SECRET_DOOR_NORTH);
}

void MapRecord::OnBumpInFog(cs::Ref<SharedMap> map, cs::Ref<Floor> fl, int32_t x, int32_t y, Facing facing) {
    if (x < 0 || y < 0 || x >= fl->Width || y >= fl->Height) return;
    if (!fl->Has(x, y, Floor::FOG)) return;
    int32_t cx, cy, bit;
    if (facing == Facing::North) { cx = x; cy = y; bit = Floor::WALL_NORTH; }
    else if (facing == Facing::East) { cx = x; cy = y; bit = Floor::WALL_EAST; }
    else if (facing == Facing::South) { cx = x; cy = y - 1; bit = Floor::WALL_NORTH; }
    else { cx = x - 1; cy = y; bit = Floor::WALL_EAST; }
    if (cx < 0 || cy < 0 || cx >= fl->Width || cy >= fl->Height) return;
    map->Record(fl->LevelNum, cx, cy, fl->Width, bit);
}

void MapRecord::BackfillExplored(cs::Ref<SharedMap> map, cs::Ref<Floor> fl, cs::List<Tup_x_y_82717> explored) {
    int32_t level = fl->LevelNum, w = fl->Width;
    for (auto& _p : explored) {
        int32_t x = _p.x, y = _p.y;
        if (x < 0 || y < 0 || x >= fl->Width || y >= fl->Height) continue;
        int64_t truth = fl->At(x, y).Flags;

        for (int32_t bit : Features)
            if ((truth & (1LL << bit)) != 0) map->Record(level, x, y, w, bit);

        if ((truth & (1LL << Floor::FOG)) != 0) continue;

        for (int32_t bit : OwnEdges)
            if ((truth & (1LL << bit)) != 0) map->Record(level, x, y, w, bit);

        // a secret door the old map was drawing is written back as the wall it should have been
        for (int32_t bit : SecretEdges)
            if ((truth & (1LL << bit)) != 0) map->Record(level, x, y, w, WallBehind(bit));

        if (x == fl->Width - 1) map->Record(level, x, y, w, Floor::WALL_EAST);
        if (y == fl->Height - 1) map->Record(level, x, y, w, Floor::WALL_NORTH);
    }
}

void MapRecord::OnPassingThrough(cs::Ref<SharedMap> map, cs::Ref<Floor> fl, int32_t x, int32_t y, Facing dir) {
    int32_t w = fl->Width, level = fl->LevelNum;
    int32_t cx, cy, wallBit, doorBit, secretBit;
    if (dir == Facing::East) { cx = x; cy = y; wallBit = Floor::WALL_EAST; doorBit = Floor::DOOR_EAST; secretBit = Floor::SECRET_DOOR_EAST; }
    else if (dir == Facing::North) { cx = x; cy = y; wallBit = Floor::WALL_NORTH; doorBit = Floor::DOOR_NORTH; secretBit = Floor::SECRET_DOOR_NORTH; }
    else if (dir == Facing::West) { cx = x - 1; cy = y; wallBit = Floor::WALL_EAST; doorBit = Floor::DOOR_EAST; secretBit = Floor::SECRET_DOOR_EAST; }
    else { cx = x; cy = y - 1; wallBit = Floor::WALL_NORTH; doorBit = Floor::DOOR_NORTH; secretBit = Floor::SECRET_DOOR_NORTH; }
    if (cx < 0 || cy < 0 || cx >= fl->Width || cy >= fl->Height) return;

    int64_t truth = fl->At(cx, cy).Flags;
    bool secret = (truth & (1LL << secretBit)) != 0;
    bool door = (truth & (1LL << doorBit)) != 0;
    if (!secret && !door) return;

    map->Record(level, cx, cy, w, secret ? secretBit : doorBit);
    map->Unrecord(level, cx, cy, w, wallBit);
}

void MapRecord::Square(cs::Ref<SharedMap> map, cs::Ref<Floor> fl, cs::Ref<Character> c, int32_t level, int32_t w, int32_t x, int32_t y,
                       cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> gear, bool foggy, bool isOwn,
                       cs::List<int32_t> only, int32_t secret) {
    int64_t truth = fl->At(x, y).Flags;

    if (isOwn) {
        // the features go down whatever the weather
        for (int32_t bit : Features)
            if ((truth & (1LL << bit)) != 0) map->Record(level, x, y, w, bit);

        // MAZEMOD.BAS:1618-1623 - the far edges of the grid are walls, and you can see that
        if (!foggy) {
            if (x == fl->Width - 1) map->Record(level, x, y, w, Floor::WALL_EAST);
            if (y == fl->Height - 1) map->Record(level, x, y, w, Floor::WALL_NORTH);
        }
    }

    if (foggy) return;   // in fog the edges are not written down at all

    for (int32_t bit : (only ? only : OwnEdges))
        if ((truth & (1LL << bit)) != 0) map->Record(level, x, y, w, bit);

    for (int32_t bit : (secret >= 0 ? cs::List<int32_t>::Of({secret}) : SecretEdges)) {
        if ((truth & (1LL << bit)) == 0) continue;
        if (map->Recorded(level, x, y, w, bit)) continue;    // already found, nothing to roll for

        int32_t wall = WallBehind(bit);
        if (NoticesSecretDoor(c, rng, gear)) {
            map->Record(level, x, y, w, bit);
            map->Unrecord(level, x, y, w, wall);
        } else map->Record(level, x, y, w, wall);
    }
}

}  // namespace mc
