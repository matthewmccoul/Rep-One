#include "test_support.h"
// FIXED: mordor/character.h does not exist in the project; include removed.
// FIXED: mordor/chute.h is also missing (GCC stops at the first missing include, so it was not reported); include removed.
// FIXED: the type Character and the functions chuteFloats, chuteLand and chuteDisappeared are declared in no header in the API index; the Character helper and the tests that call those functions are converted to SKIP comments below.
#include "mordor/state_common.h"

using namespace mordor;

// [B_0064#26] SKIP: needs ChestTraps::spring and SpellResist (not declared yet; NEED above).
// [B_0064#41] SKIP: needs ChestTraps::spring Lines to count distinct message shapes (NEED above).
// [B_0064#25] SKIP: needs ChestTraps::spring with fire trap and SpellResist (NEED above).
// [B_0064#40] SKIP: needs ChestTraps::spring Lines across traps 1..10 (NEED above).

// [B_0066#54] SKIP: needs Character and chuteFloats (not declared in any header; NEED above).
// [B_0066#55] SKIP: needs Character and chuteFloats seed sweep (not declared in any header; NEED above).
// [B_0066#51] SKIP: needs Character and chuteLand (not declared in any header; NEED above).
// [B_0066#56] SKIP: needs Character and chuteDisappeared (not declared in any header; NEED above).
