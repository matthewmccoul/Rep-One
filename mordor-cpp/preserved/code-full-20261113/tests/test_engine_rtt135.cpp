#include "test_support.h"
#include "mordor/state_common.h"

using namespace mordor;

// SKIP B_0064#85: needs CompanionCarry::Run (tie case) and the companion-carry size lookup; no declaration in our headers.
// SKIP B_0064#99: needs CompanionCarry::Run (tooBig case), the size lookup and the owner-on-screen message text; no declaration in our headers.
// SKIP B_0059#70: needs CompanionSlots (Release/Left) plus JSON save/load round trip; neither is declared in our headers.
// SKIP B_0059#69: needs CompanionSlots (Release/Left) plus JSON save/load round trip; neither is declared in our headers.
// SKIP B_0055#17: needs the CompanionSlots.NoRearrangingTheDead message constant/emitter; not declared in our headers.
// SKIP B_0059#68: needs CompanionSlots (Release) plus JSON save/load round trip; neither is declared in our headers.
// SKIP B_0055#27: needs CompanionSlots::Settle over Monster table and Companion records (Id, Ref, Alignment); not declared in our headers.
// SKIP B_0055#44: needs CompanionSlots::Settle / CompanionRef::Settle resolving a Ref to a Monster name; not declared in our headers.
