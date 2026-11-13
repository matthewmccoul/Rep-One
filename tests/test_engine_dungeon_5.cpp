#include "test_support.h"
#include "mordor/dungeon1b.h"

using namespace mordor;

// SKIP B_0063#118: needs FloorTeleporter/FloorField member access to find random-destination teleporters; not in the headers.
// SKIP B_0063#119: needs a simulation that follows fixed teleporter destinations; no equivalent function.
// SKIP B_0063#120: needs counting of fixed destinations and teleporter bits across data; no equivalent function.
// SKIP B_0063#121: needs square flag access (water, quicksand, extinguisher, etc.); FloorField members not in the headers.
// SKIP B_0063#122: needs per-square water and quicksand flag access; FloorField members not in the headers.
// SKIP B_0063#123: needs chute and square-effect flag access; FloorField members not in the headers.

// SKIP B_0063#126: needs a FloorTeleporter configured as random-destination; its fields are not in the headers.
// SKIP B_0063#127: needs a 200-jump simulation with splitTeleportedParty and a random-destination FloorTeleporter setup; fields not in the headers.
