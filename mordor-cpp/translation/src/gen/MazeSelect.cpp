#include "core.h"

namespace mc {

int32_t MazeSelect_View::Square() { return X + MazeSelect::MapWidth * (Y - 1); }

int32_t MazeSelect::Bit(int32_t bit, int32_t word) { return (word & (1 << bit)) != 0 ? 1 : 0; }

int32_t MazeSelect::SetBit(int32_t bit, int32_t word, int32_t set) {
    return set != 0 ? word | (1 << bit)
                    : Bit(bit, word) == 1 ? word ^ (1 << bit) : word;
}

int32_t MazeSelect::WallsAt(cs::Ref<MazeSelect_View> v, int32_t square) { return v->Walls(square); }

cs::Ref<MazeSelect_View> MazeSelect::For(int32_t x, int32_t y, int32_t facing, int32_t level, cs::Func<int32_t(int32_t)> walls) {
    auto v = [&] { auto _o = cs::New<MazeSelect_View>(); _o->X = x; _o->Y = y; _o->Facing = facing; _o->Level = level; _o->Walls = walls; return _o; }();
    Select(v);
    return v;
}

}  // namespace mc
