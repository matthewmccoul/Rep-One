#include "test_support.h"
#include "mordor/companions1b.h"
#include "mordor/state_companion.h"
#include "mordor/state_character.h"

using namespace mordor;

// SKIP B_0064_88: depends on the earlier setup of case B_0064#87 (not provided); carry state before this case is unknown.
// SKIP B_0064_89: depends on the earlier setup of case B_0064#87 (not provided); slot state before this case is unknown.
// SKIP B_0064_90: asserts emitted message text; EventLog accessors are not in the headers, and owner visibility has no parameter.
// SKIP B_0064_93: asserts message count; EventLog accessors are not in the headers, and a morgue-body carry has no header representation.
// SKIP B_0064_94: morgue-body carry state and the meaning of "held" for kept slots are not representable with the given headers.
// SKIP B_0064_95: a morgue-body carry (carried name "Bob") has no header field that can be asserted reliably.
// SKIP B_0064_96: asserts emitted message text joined by '|'; EventLog accessors are not in the headers.

