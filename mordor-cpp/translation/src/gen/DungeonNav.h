#pragma once
#include "fwd.h"
#include "enums.h"
#include "tuples.h"

namespace mc {

struct DungeonNav {
    cs::Ref<mc::Floor> Floor;
    int32_t X = 0;
    int32_t Y = 0;
    mc::Facing Facing{};
    cs::HashSet<Tup_x_y_82717> Explored = cs::HashSet<Tup_x_y_82717>::New();
    bool Mapping = true;

    DungeonNav(cs::Ref<mc::Floor> floor, int32_t x, int32_t y, mc::Facing facing = mc::Facing::North);

    static Tup_dx_dy_0fdb7 Delta(mc::Facing f);
    static mc::Facing TurnRight(mc::Facing f);
    static mc::Facing TurnLeft(mc::Facing f);
    static mc::Facing Opposite(mc::Facing f);
    bool InBounds(int32_t x, int32_t y);
    Edge EdgeAt(int32_t x, int32_t y, mc::Facing f);
    static Edge Classify(int64_t flags, int32_t wallBit, int32_t doorBit, int32_t secretBit);
    bool IsRock(int32_t x, int32_t y);
    bool CanMove(mc::Facing f);
    bool MoveForward();
    bool MoveBackward();
    void TurnLeft();
    void TurnRight();
    void TurnAround();
    void SetPosition(int32_t x, int32_t y);
    bool StepThrough();
    void MarkExplored();
    void FaceTowards(mc::Facing f);
    bool HasMapped(int32_t x, int32_t y);
    void MapHere();
    cs::String Describe();
};

}  // namespace mc
