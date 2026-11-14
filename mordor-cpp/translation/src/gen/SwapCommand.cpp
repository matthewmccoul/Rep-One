#include "core.h"

namespace mc {

cs::Nullable<Tup_A_B_cbb7a> SwapCommand::Parse(cs::String typed) {
    cs::String s = cs::Str::Trim(typed);
    if (s == "" || !cs::Str::Contains(s, ',') || (int32_t)s.size() > 5) return std::nullopt;
    auto parts = cs::Str::Split(s, ',');
    if (parts.Length() != 2) return std::nullopt;
    int32_t a = 0;
    if (!cs::Parse::TryParse<int32_t>(cs::Str::Trim(parts[0]), a)) return std::nullopt;
    int32_t b = 0;
    if (!cs::Parse::TryParse<int32_t>(cs::Str::Trim(parts[1]), b)) return std::nullopt;
    return Tup_A_B_cbb7a{a, b};
}

}  // namespace mc
