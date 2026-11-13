// Batch t164: translated from reference assertions (B_0066, B_0063).
// All cases are pending the NEED declarations listed above; no tests are written yet.
#include "test_support.h"

using namespace mordor;

// PENDING B_0066#67 (Party.HereToJoin): stale record on lower floor carries upper area id; expected 0 listed.
// PENDING B_0066#63 (Party.Remove): Fell removed from Lead,Mid,Fell; expected member list Lead,Mid.
// PENDING B_0066#66 (Party.WhyCannotJoin): Fell on chute square, same area, same square; expected no reason ("joins").
// PENDING B_0063#87 (PerceiveAhead.Look): quicksand ahead on firm ground at perception 100; expected "You detect Quicksand ahead of you."
// PENDING B_0063#88 (PerceiveAhead.Look): same setup; expected quicksand square written to shared map (true).
// PENDING B_0063#89 (PerceiveAhead.Look): standing in quicksand with quicksand ahead; expected 0 lines said.
// PENDING B_0063#90 (PerceiveAhead.Look): standing in quicksand with quicksand ahead; expected quicksand ahead written to shared map (true).
// PENDING B_0063#91 (PerceiveAhead.Look): standing in quicksand with pit ahead; expected "You detect a Pit ahead of you."
