#include "test_support.h"

// SKIP B_0065#29: needs a multi-round chest-opening simulation; no header function maps per-round chest-open line output.
// SKIP B_0065#30: needs multi-round chest-count tracking for m1; the headers do not define how per-round counts decrement.
// SKIP B_0065#31: needs multi-round opener selection across rounds; no header function reports which characters open the chest per round.
// SKIP B_0065#32: needs ChestEmptied fight state and per-round chest count for m0; no header function models this sequence.
// SKIP B_0065#33: needs ChestEmptied fight state and counts chest-open lines per round; no header function produces these lines.
// SKIP B_0066#114: needs returning to an emptied room on floor 2 after elapsed time; no header function handles room return or respawn.
// SKIP B_0066#117: needs 200 joined encounters; no header function joins encounters, and the tuning inputs are not given in the case.
// SKIP B_0066#118: needs joined encounters paying owed gold to the character; no header function performs this join-and-pay step.
// SKIP B_0066#119: needs 200 joined encounters; no header function joins encounters, and the tuning inputs are not given in the case.
// SKIP B_0066#120: needs joined encounters that move owed gold into the chest; no header function performs this step.
