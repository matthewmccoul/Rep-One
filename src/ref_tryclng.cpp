// ref: src/Mordor.Core/VbConvert.cs:22 (VbConvert.TryCLng)
#include "mordor/ref_tryclng.h"

#include <cstddef>
#include <cstdlib>
#include <string>

#include "mordor/math_util.h"  // ASSUMES double roundHalfEven(double) = SRD round_half_even

namespace mordor {
namespace {

// Half-open span [begin, end) of the text after trimming.
struct Span {
    std::size_t begin;
    std::size_t end;
};

}  // namespace

}  // namespace mordor
