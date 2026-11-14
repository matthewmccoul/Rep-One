#include "core.h"

namespace mc {

DungeonNav::DungeonNav(cs::Ref<mc::Floor> floor, int32_t x, int32_t y, mc::Facing facing) {
    Floor = floor; X = x; Y = y; Facing = facing;
    MarkExplored();
}

Tup_dx_dy_0fdb7 DungeonNav::Delta(mc::Facing f) {
    switch (f) {
        case mc::Facing::North: return Tup_dx_dy_0fdb7{0, 1};
        case mc::Facing::East: return Tup_dx_dy_0fdb7{1, 0};
        case mc::Facing::South: return Tup_dx_dy_0fdb7{0, -1};
        default: return Tup_dx_dy_0fdb7{-1, 0};
    }
}

mc::Facing DungeonNav::TurnRight(mc::Facing f) { return static_cast<mc::Facing>((static_cast<int32_t>(f) + 1) & 3); }
mc::Facing DungeonNav::TurnLeft(mc::Facing f) { return static_cast<mc::Facing>((static_cast<int32_t>(f) + 3) & 3); }
mc::Facing DungeonNav::Opposite(mc::Facing f) { return static_cast<mc::Facing>((static_cast<int32_t>(f) + 2) & 3); }

bool DungeonNav::InBounds(int32_t x, int32_t y) {
    return x >= 0 && y >= 0 && x < Floor->Width && y < Floor->Height;
}

Edge DungeonNav::EdgeAt(int32_t x, int32_t y, mc::Facing f) {
    if (!InBounds(x, y)) return Edge::Wall;
    switch (f) {
        case mc::Facing::North:
            return Classify(Floor->At(x, y).Flags, mc::Floor::WALL_NORTH, mc::Floor::DOOR_NORTH, mc::Floor::SECRET_DOOR_NORTH);
        case mc::Facing::East:
            return Classify(Floor->At(x, y).Flags, mc::Floor::WALL_EAST, mc::Floor::DOOR_EAST, mc::Floor::SECRET_DOOR_EAST);
        case mc::Facing::South:
            return InBounds(x, y - 1)
                ? Classify(Floor->At(x, y - 1).Flags, mc::Floor::WALL_NORTH, mc::Floor::DOOR_NORTH, mc::Floor::SECRET_DOOR_NORTH)
                : Edge::Wall;
        default: // West
            return InBounds(x - 1, y)
                ? Classify(Floor->At(x - 1, y).Flags, mc::Floor::WALL_EAST, mc::Floor::DOOR_EAST, mc::Floor::SECRET_DOOR_EAST)
                : Edge::Wall;
    }
}

Edge DungeonNav::Classify(int64_t flags, int32_t wallBit, int32_t doorBit, int32_t secretBit) {
    if ((flags & ((int64_t)1 << (wallBit & 63))) != 0) return Edge::Wall;
    if ((flags & ((int64_t)1 << (doorBit & 63))) != 0) return Edge::Door;
    if ((flags & ((int64_t)1 << (secretBit & 63))) != 0) return Edge::SecretDoor;
    return Edge::Open;
}

bool DungeonNav::IsRock(int32_t x, int32_t y) {
    return !InBounds(x, y) || Floor->Has(x, y, mc::Floor::ROCK);
}

bool DungeonNav::CanMove(mc::Facing f) {
    auto e = EdgeAt(X, Y, f);
    if (e == Edge::Wall) return false;
    auto d = Delta(f);
    auto dx = d.dx; auto dy = d.dy;
    return !IsRock(X + dx, Y + dy);
}

bool DungeonNav::MoveForward() {
    if (!CanMove(Facing)) return false;
    auto d = Delta(Facing);
    auto dx = d.dx; auto dy = d.dy;
    X += dx; Y += dy;
    MarkExplored();
    return true;
}

bool DungeonNav::MoveBackward() {
    auto back = Opposite(Facing);
    if (!CanMove(back)) return false;
    auto d = Delta(back);
    auto dx = d.dx; auto dy = d.dy;
    X += dx; Y += dy;
    MarkExplored();
    return true;
}

void DungeonNav::TurnLeft() { Facing = TurnLeft(Facing); }
void DungeonNav::TurnRight() { Facing = TurnRight(Facing); }
void DungeonNav::TurnAround() { Facing = Opposite(Facing); }

void DungeonNav::SetPosition(int32_t x, int32_t y) {
    X = x; Y = y;
    if (!IsRock(x, y)) MarkExplored();
}

bool DungeonNav::StepThrough() {
    auto d = Delta(Facing);
    auto dx = d.dx; auto dy = d.dy;
    int32_t nx = X + dx, ny = Y + dy;
    if (nx >= Floor->Width) nx = 0;
    if (nx < 0) nx = Floor->Width - 1;
    if (ny >= Floor->Height) ny = 0;
    if (ny < 0) ny = Floor->Height - 1;
    SetPosition(nx, ny);
    return IsRock(nx, ny);
}

void DungeonNav::MarkExplored() {
    if (!Mapping) return;              // you cannot map ground you do not know you are on
    Explored.Add(Tup_x_y_82717{X, Y});
}

void DungeonNav::FaceTowards(mc::Facing f) { while (Facing != f) TurnRight(); }

bool DungeonNav::HasMapped(int32_t x, int32_t y) { return Explored.Contains(Tup_x_y_82717{x, y}); }

void DungeonNav::MapHere() { MarkExplored(); }

cs::String DungeonNav::Describe() { return SquareInfo::Of(Floor, X, Y); }

}  // namespace mc
