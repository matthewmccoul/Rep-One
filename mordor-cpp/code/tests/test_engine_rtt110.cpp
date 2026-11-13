#include "test_support.h"
#include "mordor/state_common.h"

using namespace mordor;

// SKIP B_0066#122: Character.Paralyse / stat part of Att/Def: no Character type, paralyse() or StatsADNow() declared in our headers (only storedADShift(const Character&) in combat3.h).
// SKIP B_0066#125: Character.Paralyse with a level-0 monster: needs Character, paralyse(), ADShift and ParalysisLevel accessors, not declared.
// SKIP B_0066#126: second level-0 paralysis stat part: needs Character and paralyse(), not declared.
// SKIP B_0066#129: RestInCity + Save/reload round trip: persistence is file I/O, which the core never does (the port's save layer is outside the core); also needs Character.
// SKIP B_0066#128: Save/Load round trip of an old held save: file I/O is outside the core; also needs Character.
// SKIP B_0066#121: StatsADNow + StoredADShift stat part with all four stats at 18: needs Character, StatsADNow() and StoredADShift(item) declarations; our storedADShift(const Character&) returns the stored shift, not the stat part.
// SKIP B_0066#127: Unparalyse after a Str raise to 20 while held: needs Character, unparalyse(), stat setters and ADShift accessor, not declared.
// SKIP B_0066#130: Unparalyse after Save/ADShift-line removal/Load round trips: file I/O is outside the core; also needs Character and unparalyse().

