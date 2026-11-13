#include "test_support.h"

#include "mordor/types.h"
#include "mordor/ctx.h"
#include "mordor/state_character.h"
#include "mordor/state_party.h"
#include "mordor/death1a.h"
#include "mordor/death1b.h"

// SKIP B_0068#10: "Hire Rescuers" button visibility on the city dead screen is UI; no header function reports which buttons are offered.
// SKIP B_0068#11: "Visit the Morgue" button visibility on the city dead screen is UI; no header function reports which buttons are offered.
// SKIP B_0068#12: "any button offered" on the rock-death dead screen is UI; deadScreenOwed() only reports whether the screen is owed, not which buttons appear.
// SKIP A_03#23: the importer that maps raise, kill, death and play-time fields from a save file has no equivalent in these headers.
// SKIP A_03#36: the importer that reads Hall of Records entries (MDATA14) has no equivalent in these headers; HallOfRecords is only a runtime structure.
