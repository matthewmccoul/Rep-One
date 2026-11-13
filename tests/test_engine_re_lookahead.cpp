#include "test_support.h"
#include "mordor/dungeon1a.h"
// NEED: PerceiveAhead look function, SharedMap, and Floor query API (see NEED line above)

using namespace mordor;

// SKIP B_0063#87: our code has no perceive-ahead Look function declared (NEED: lookahead_look); cannot call it to get the announced lines.
// SKIP B_0063#88: needs SharedMap record store and Floor feature query, neither declared (NEED: SharedMap, Floor); cannot check the map write.
// SKIP B_0063#89: needs PerceiveAhead Look and the standing-in-quicksand suppression rule, neither declared (NEED: lookahead_look); cannot count lines.
// SKIP B_0063#90: needs SharedMap record store and Floor feature query, neither declared (NEED: SharedMap, Floor); cannot check the map write.
// SKIP B_0063#91: needs PerceiveAhead Look, neither declared (NEED: lookahead_look); cannot check the pit announcement.
