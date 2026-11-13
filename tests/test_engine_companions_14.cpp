#include "test_support.h"

// SKIP B_0065#91: join-of-two binding level needs acceptJoinOffer simulation with a group of two; outcome of the scripted join roll and which slots fill are not specified by the headers.
// SKIP B_0065#92: remaining monster count after a join of two is an acceptJoinOffer group-state outcome not specified by the headers.
// SKIP B_0065#93: "No room for more Companions!" is an event text; EventLog has no accessor shown in the headers to inspect emitted text.
// SKIP B_0065#94: offer count on a refused join depends on group bookkeeping not specified by the headers.
// SKIP B_0065#95: two-group join needs multiple groups, but acceptJoinOffer takes a single MonsterGroup; no equivalent call.
// SKIP B_0065#103: "raised the Bind Level of companion #3 to" is an event text; no EventLog accessor is shown to check it.
// SKIP B_0065#104: "had no effect!" is an event text; no EventLog accessor is shown to check it.
// SKIP B_0065#105: "Companion #1 is now bound to you for life!" is an event text; no EventLog accessor is shown to check it.
// SKIP B_0066#44: CompanionInfo.Paint text at a grid position is UI rendering; no equivalent function in the headers.
// SKIP B_0066#45: CompanionInfo.Paint text at a grid position is UI rendering; no equivalent function in the headers.
