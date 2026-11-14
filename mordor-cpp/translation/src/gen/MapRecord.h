#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct MapRecord {
    static cs::List<int32_t> OwnEdges;
    static cs::List<int32_t> SecretEdges;
    static cs::List<int32_t> Features;

    static int32_t WallBehind(int32_t secretBit);
    static bool NoticesSecretDoor(cs::Ref<Character> c, cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> gear = {});
    static void OnEntry(cs::Ref<SharedMap> map, cs::Ref<Floor> fl, cs::Ref<Character> c, int32_t x, int32_t y,
                        cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> gear = {});
    static void OnBumpInFog(cs::Ref<SharedMap> map, cs::Ref<Floor> fl, int32_t x, int32_t y, Facing facing);
    static void BackfillExplored(cs::Ref<SharedMap> map, cs::Ref<Floor> fl, cs::List<Tup_x_y_82717> explored);
    static void OnPassingThrough(cs::Ref<SharedMap> map, cs::Ref<Floor> fl, int32_t x, int32_t y, Facing dir);

private:
    static void Square(cs::Ref<SharedMap> map, cs::Ref<Floor> fl, cs::Ref<Character> c, int32_t level, int32_t w, int32_t x, int32_t y,
                       cs::Ref<cs::Random> rng, cs::Func<cs::Ref<Item>(int16_t)> gear, bool foggy, bool isOwn,
                       cs::List<int32_t> only = nullptr, int32_t secret = -1);
};

}  // namespace mc
