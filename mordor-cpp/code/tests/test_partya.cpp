#include "test_support.h"
#include "mordor/partyb.h"
#include "mordor/partya.h"

using namespace mordor;

namespace {

Party partyOf(i32 n) {
    Party p;
    p.size = n;
    return p;
}

}  // namespace

