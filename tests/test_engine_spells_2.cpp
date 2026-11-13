#include "test_support.h"
#include "mordor/spells1a.h"
#include "mordor/spells2.h"

using namespace mordor;

// SKIP B_0061#54: the cast-time Protection value is computed from the caster's spell level, and the
// new API does not state how a Wizard at guild level 26 maps to that spell level.

// SKIP B_0062#53: needs MDATA2 spell loading and iteration over spell definitions; no loader/spell-list
// function appears in the headers.
// SKIP B_0062#54: needs character creation and spellbook castables query; no equivalent in headers.
// SKIP B_0062#55: needs character creation, pack placement and castables query; no equivalent in headers.
// SKIP B_0062#56: needs loading spell definitions and enumerating class-targeted spells; no loader in headers.
// SKIP B_0062#57: needs full combat cast of each class-targeted spell against monster rosters; the
//                 spell list is not available through the headers.
// SKIP B_0062#58: needs full combat cast of each class-targeted spell against other-class monsters; the
//                 spell list is not available through the headers.
