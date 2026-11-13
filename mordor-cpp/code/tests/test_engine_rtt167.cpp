#include "test_support.h"
#include "mordor/guilds2.h"

using namespace mordor;

// SKIP B_0061#159: needs Character, Guild and GuildQuest layouts plus the guild table
// (QuestPercent per guild) to find the most generous guild and count 2000 RollQuest results; not declared.

// SKIP B_0063#126: no declaration of the teleporter square draw (ForSquare equivalent); only TeleporterDest
// is declared in dungeon1b.h and its fields and the Floor/map type are not shown.

// SKIP B_0063#127: same as B_0063#126 - the party-split draw over 200 jumps needs the teleport destination
// function and the Floor/map type, which are not declared.

// SKIP B_0063#128: same as B_0063#126 - fixed-destination level-change teleport needs the teleport
// destination function and the Floor/map type, which are not declared.
