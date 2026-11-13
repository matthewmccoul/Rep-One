#include <string>

#include "mordor/access.h"
#include "mordor/features.h"
#include "mordor/math_util.h"
#include "mordor/state_common.h"
#include "mordor/types.h"

namespace mordor {

// Resolves a disarm attempt whose roll is already known (success = true means the trap came out).
// Exported so the smoke test can drive the state change directly. Returns true when a trap was resolved.
bool disarmFinish(Session& s, i32 member, bool success, i32 monsterGuildLevel, Str& reply);

namespace {

// Trap names as the original prints them (gv08AA$, MODULE1.BAS:408-418), indexed by trap number.
const char* disarmTrapName(i32 trap) {
  switch (trap) {
    case 0: return "Not Trapped";
    case 1: return "Poison Gas";
    case 2: return "Disease Spores";
    case 3: return "Exploding Trap";
    case 4: return "Fire Trap";
    case 5: return "Slime";
    case 6: return "Fate";
    case 7: return "Teleporter";
    case 8: return "Blackout";
    case 9: return "Fear";
    case 10: return "Withering";
    default: return "Unknown Trap";
  }
}

// Parses a party member number from words[1] and checks it against the party size.
bool disarmParseMember(const Session& s, const Vec<Str>& w, i32& member) {
  if (w.size() < 2) return false;
  i64 value = 0;
  if (!accParseInt(w[1], value)) return false;
  if (value < 1 || value > accPartySize(s)) return false;
  member = static_cast<i32>(value);
  return true;
}

// The chest on the square under the party. nullptr in town, or when there is no chest.
SquareState* disarmChestHere(Session& s) {
  SquareState* sq = accSquareHere(s);
  return (sq != nullptr && sq->hasChest) ? sq : nullptr;
}

bool cmdInspect(Session& s, const Vec<Str>& w, Str& reply) {
  (void)w;
  const SquareState* chest = disarmChestHere(s);
  if (chest == nullptr) {
    reply = "There is no chest here.";
    return true;
  }
  const i32 trap = static_cast<i32>(chest->chestTrap);
  if (trap == 0) {
    reply = "This chest is not trapped.";
    return true;
  }
  // UNKNOWN: whether the name is gated by the opener's skill. The reference shows the
  // name in lblTrap; the skill gate is not in the evidence, so the name is revealed.
  reply = Str("This chest is trapped: ") + disarmTrapName(trap) + ".";
  return true;
}

bool cmdDisarm(Session& s, const Vec<Str>& w, Str& reply) {
  i32 member = 0;
  if (!disarmParseMember(s, w, member)) {
    reply = "Usage: disarm <member>";
    return true;
  }
  const SquareState* chest = disarmChestHere(s);
  if (chest == nullptr) {
    reply = "There is no chest here.";
    return true;
  }
  const i32 trap = static_cast<i32>(chest->chestTrap);
  if (trap == 0) {
    reply = "This chest is not trapped.";
    return true;
  }
  // The roll itself (rollTrapDisarm, chance = chest->chestDisarm) needs a Ctx for the rng.
  // This command receives only the Session, so it cannot draw the roll yet. Nothing is changed.
  reply = "The disarm roll cannot be made from here yet; the chest is untouched.";
  return true;
}

}  // namespace

void register_disarm() {
  registerFeatureCommand({"inspect", Screen::Dungeon, &cmdInspect, "inspect  - tells what trap the chest on this square has"});
  registerFeatureCommand({"disarm", Screen::Dungeon, &cmdDisarm, "disarm <member>  - tries to disarm the chest trap on this square"});
}

}  // namespace mordor
