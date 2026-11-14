#include "core.h"

namespace mc {

cs::List<int32_t> PerceiveAhead::Features = cs::List<int32_t>::Of({
    Floor::QUICKSAND, Floor::STAIRS_UP, Floor::STAIRS_DOWN, Floor::TELEPORTER,
    Floor::CHUTE, Floor::PIT, Floor::ROTATOR, Floor::EXTINGUISHER,
});

cs::String PerceiveAhead::Alone(int32_t bit) {
    cs::String name;
    if (bit == Floor::QUICKSAND) name = "Quicksand";
    else if (bit == Floor::STAIRS_UP) name = "Stairs Up";
    else if (bit == Floor::STAIRS_DOWN) name = "Stairs Down";
    else if (bit == Floor::TELEPORTER) name = "a Teleporter";
    else if (bit == Floor::CHUTE) name = "a Chute";
    else if (bit == Floor::PIT) name = "a Pit";
    else if (bit == Floor::ROTATOR) name = "a rotator";
    else name = "an Extinguish square";
    return "You detect " + name + " ahead of you.";
}

cs::String PerceiveAhead::InParty(cs::String who, int32_t bit) {
    cs::String name;
    if (bit == Floor::QUICKSAND) name = "Quicksand";
    else if (bit == Floor::STAIRS_UP) name = "Stairs Up";
    else if (bit == Floor::STAIRS_DOWN) name = "Stairs Down";
    else if (bit == Floor::TELEPORTER) name = "a Teleporter";
    else if (bit == Floor::CHUTE) name = "a Chute";
    else if (bit == Floor::PIT) name = "a Pit";
    else if (bit == Floor::ROTATOR) name = "a Rotator";
    else name = "an Extinguish Square";
    return "\"" + cs::Str::TrimEnd(who) + "\" detects " + name + " ahead of the party.";
}

cs::Nullable<Tup_x_y_82717> PerceiveAhead::SquareAhead(cs::Ref<Floor> fl, int32_t x, int32_t y, Facing facing) {
    auto Blocked = [&](int32_t cx, int32_t cy, bool northSide) -> bool {
        if (cx < 0 || cy < 0 || cx >= fl->Width || cy >= fl->Height) return true;
        int64_t f = fl->At(cx, cy).Flags;
        int32_t wall = northSide ? Floor::WALL_NORTH : Floor::WALL_EAST;
        int32_t door = northSide ? Floor::DOOR_NORTH : Floor::DOOR_EAST;
        int32_t secret = northSide ? Floor::SECRET_DOOR_NORTH : Floor::SECRET_DOOR_EAST;
        return (f & ((1LL << wall) | (1LL << door) | (1LL << secret))) != 0;
    };

    int32_t tx = x, ty = y;
    switch (facing) {
        case Facing::North:
            if (Blocked(x, y, true)) return std::nullopt;
            ty++; break;
        case Facing::East:
            if (Blocked(x, y, false)) return std::nullopt;
            tx++; break;
        case Facing::South:
            if (y <= 0 || Blocked(x, y - 1, true)) return std::nullopt;
            ty--; break;
        default:
            if (x <= 0 || Blocked(x - 1, y, false)) return std::nullopt;
            tx--; break;
    }
    if (tx < 0 || ty < 0 || tx >= fl->Width || ty >= fl->Height) return std::nullopt;
    return Tup_x_y_82717{tx, ty};
}

cs::List<cs::String> PerceiveAhead::Look(cs::Ref<SharedMap> map, cs::Ref<Floor> fl, int32_t x, int32_t y, Facing facing,
                                         cs::Ref<cs::Random> rng, double bestPerception, cs::String who, bool inParty) {
    auto said = cs::List<cs::String>::New();
    auto ahead = SquareAhead(fl, x, y, facing);
    if (!ahead.has_value()) return said;
    int32_t ax = ahead.value().x;
    int32_t ay = ahead.value().y;

    bool standingInQuicksand = fl->Has(x, y, Floor::QUICKSAND);

    for (auto& bit : Features) {
        if (!fl->Has(ax, ay, bit)) continue;
        if (!(rng->NextDouble() * 100 < bestPerception)) continue;

        // fn0ABD writes it into the automap file, found or not said
        map->Record(fl->LevelNum, ax, ay, fl->Width, bit);
        map->Note(fl->LevelNum, ax, ay, fl->Width, bit);

        if (bit == Floor::QUICKSAND && standingInQuicksand) continue;
        said.Add(inParty ? InParty(who, bit) : Alone(bit));
    }
    return said;
}

Tup_best_who_d260c PerceiveAhead::Best(cs::Ref<Party> party, int32_t level, cs::Func<cs::Ref<Item>(int16_t)> gear) {
    double best = 0;
    cs::Ref<Character> who = party->Members[0];
    for (auto& m : party->Members) {
        double p = Rules::BestAbilityChance(m, 5, level, 0, gear);
        if (p > best) { best = p; who = m; }
    }
    return Tup_best_who_d260c{cs::Math::Min(best, 95.0), who};
}

}  // namespace mc
