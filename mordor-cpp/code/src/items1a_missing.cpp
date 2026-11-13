// items1a_missing.cpp
// Implements: giveFromPack, giveFromVault, itemDisplayName (items1a.h).
// ASSUMED: GameData field names used here (items[].name, itemTypes[].name,
// itemDescriptions[].name) are not given in the foundation summary; they follow
// the data-file naming and must be checked against game_data.h.
#include <string>

#include "mordor/items1a.h"

namespace mordor {

// R47, R48, R53: give from pack.
// File-local helper, inside namespace mordor. Mirrors C# TrimEnd() as used by
// Party.GaveItem (Party.cs:502) and Party.NoFreeSlots (Party.cs:467).
namespace {
}  // namespace

}  // namespace mordor
