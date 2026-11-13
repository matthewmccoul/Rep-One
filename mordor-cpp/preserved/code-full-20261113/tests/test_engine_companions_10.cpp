#include "test_support.h"

// SKIP B_0064#109: needs a multi-round combat loop over 3000 rounds (no Round/simulation function in headers) to count accidental hits on companions.
// SKIP B_0064#110: needs a multi-round combat loop to see which owner is named in the accidental-hit caption; no simulation function in headers.
// SKIP B_0064#111: needs a multi-round loop to compare the 'You were accidently hit for' caption with m0 hits; caption output is UI/event-log level, not exposed as a single call.
// SKIP B_0064#112: needs a multi-round loop to count caption occurrences when non-m0 characters are hit; no simulation function in headers.
// SKIP B_0064#115: needs a full CON 3 foe stoning combat sequence and the character-hit channel log; headers only expose stoneLands/applyLandedBlowEffects, not the full stoning round.
// SKIP B_0064#116: needs a full CON 40 foe stoning combat sequence and the character-hit channel log; no equivalent end-to-end function in headers.
// SKIP B_0065#1: needs a Three(zapper, false, zapper) party setup and 3000 seeded Round() calls; no Round/party-builder function in headers.
// SKIP B_0065#2: needs the multi-round electrocution simulation and the hit-line channel for m0; no simulation function in headers.
// SKIP B_0065#3: needs a 4000-round simulation of a dragon companion killing an undead companion; no Round/simulation function in headers.
// SKIP B_0065#4: needs per-round death-cry output (DI-UNDEA via CombatSounds.OfLines) across simulated kills; CombatSounds and round simulation are not in the headers.
