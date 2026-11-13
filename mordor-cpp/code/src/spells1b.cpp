#include "mordor/spells1b.h"

namespace mordor {
namespace {

// R59 reach limit. UNKNOWN: MaxDungeonLevel's value is not given in these chunks.
// 15 is used because "deeper than 15" is the only level bound the SRD names.
constexpr i32 kFetchMaxDungeonLevel = 15;

}  // namespace

}  // namespace mordor
