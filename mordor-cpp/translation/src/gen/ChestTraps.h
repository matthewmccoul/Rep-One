#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct Teleported {
    int32_t X = 0;
    int32_t Y = 0;
    bool operator==(const Teleported& o) const { return X == o.X && Y == o.Y; }
    bool operator!=(const Teleported& o) const { return !(*this == o); }
};

struct TrapResult {
    cs::List<cs::String> Lines = cs::List<cs::String>::New();
    cs::Nullable<Teleported> TeleportTo{};
};

struct ChestTraps {
    inline static cs::List<cs::String> Names = cs::List<cs::String>::Of({
        "Not Trapped", "Poison Gas", "Disease Spores", "Exploding Trap", "Fire Trap", "Slime",
        "Fate", "Teleporter", "Blackout", "Fear", "Withering",
    });

    // Resistance kinds, in the order MDATA5/MDATA3 store them.
    static constexpr int32_t ResistFire = 0, ResistDisease = 4, ResistPoison = 5;

    static constexpr int32_t WitheringDays = 28;

    static constexpr int32_t PackSlots = 40;

    static bool Destructible(cs::Ref<Item> it);

    static cs::Ref<TrapResult> Spring(
        int32_t trap, cs::List<cs::Ref<Character>> party, cs::Ref<Character> opener, int32_t dungeonLevel,
        cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> item, cs::Func<int32_t(int32_t, int32_t)> walls);
};

}  // namespace mc
