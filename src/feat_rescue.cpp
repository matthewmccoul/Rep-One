#include <cmath>
#include <cstdlib>
#include <string>

#include "mordor/access.h"
#include "mordor/attributes.h"
#include "mordor/features.h"

namespace mordor {
namespace {

// Rescue.cs constants (FRMDUNM.BAS:140-190, FRMMORG.BAS:149-171). SRD R52 WaitForRescue.
constexpr i32 kRescueCeilingEarly = 10000;     // first stretch of days
constexpr i32 kRescueCeilingLate = 1000;       // after Log(level)^2 * 100 + 21 days
constexpr i32 kRescueCeilingLevel1 = 100;      // level 1, area <= 18
constexpr i32 kRescueLevel1AreaMax = 18;
constexpr i32 kRescueDayCap = 2000000;         // Rescue.cs guard: the original would spin
constexpr i32 kRescueFinderRange = 8;          // FRMMORG.BAS:149, strictly less than 8
constexpr double kRescueBaseChance = 16.0;
constexpr double kRescueMinChance = 0.1;
constexpr double kRescueAliveFraction = 0.2;   // FRMMORG.BAS:150-171, a fifth of max hits
constexpr i32 kRescueDefaultWidth = 30;        // used when the floor has no FloorDef

// Raising.cs. What a raise costs is charged whether it works or not.
constexpr i32 kRaiseAgeOrdinary = 54;          // mc00B4
constexpr i32 kRaiseAgeStoned = 84;            // mc00C4
constexpr i32 kRaiseAgeRock = 124;             // mc00B8
constexpr i32 kComplicationAgeDays = 5475;     // mc02EA, fifteen years
constexpr double kComplicationHitsDivisor = 1.2;  // MAINMOD2.BAS:1664

// NEED: morgue rescuer fee in 1/10000 gold. -1 means unpriced; town rescue refuses until it is set.
constexpr i64 kRescueFeeUnpriced = -1;
// UNKNOWN: which member pays the morgue. The leader (member 1) is assumed.
constexpr i32 kRescuePayerMember = 1;

// Digits with an optional leading '-'. No exceptions, so no std::stoi.
bool rescueParseInt(const Str& text, i32& out) {
  if (text.empty()) return false;
  size_t i = 0;
  bool neg = false;
  if (text[0] == '-') {
    neg = true;
    i = 1;
    if (text.size() == 1) return false;
  }
  i32 v = 0;
  for (; i < text.size(); ++i) {
    const char c = text[i];
    if (c < '0' || c > '9') return false;
    v = v * 10 + static_cast<i32>(c - '0');
    if (v > 1000000000) return false;
  }
  out = neg ? -v : v;
  return true;
}

// Rescue.DaysUntilFound. Each day is one roll; days is compared before the increment, as in the reference.
i32 rescueDaysUntilFound(Ctx& ctx, i32 dungeonLevel, i32 areaId, bool easyToFind) {
  if (dungeonLevel < 1) return 0;
  const double lv = static_cast<double>(dungeonLevel);
  const double threshold = std::log(lv) * std::log(lv) * 100.0 + 21.0;
  i32 days = 0;
  while (true) {
    i32 ceiling = static_cast<double>(days) < threshold ? kRescueCeilingEarly : kRescueCeilingLate;
    if (dungeonLevel == 1 && areaId <= kRescueLevel1AreaMax) ceiling = kRescueCeilingLevel1;
    days += 1;
    double chance = kRescueBaseChance - lv;
    if (chance < kRescueMinChance) chance = kRescueMinChance;
    if (easyToFind) chance += kRescueBaseChance - lv;   // unfloored, as in the reference
    if (ctx.rng.nextDouble() * static_cast<double>(ceiling) <= chance) return days;
    if (days > kRescueDayCap) return days;
  }
}

// Rescue.CloseEnough (FRMMORG.BAS:149). Both sides use one-based coordinates.
bool rescueCloseEnough(i32 guessX, i32 guessY, i32 guessLevel, i32 x, i32 y, i32 level) {
  return std::abs(guessX - x) < kRescueFinderRange &&
         std::abs(guessY - y) < kRescueFinderRange &&
         guessLevel == level;
}

struct RescueReport {
  Str message;
  bool broughtBack = false;
};

// Rescue.Report (FRMMORG.BAS:150-171).
RescueReport rescueReport(const Str& name, bool foundAtAll, bool isDead, bool isRock,
                          i32 curHits, i32 maxHits, i32 x, i32 y, i32 level) {
  RescueReport r;
  if (!foundAtAll) {
    r.message = name + " was not found at or around the specified location.";
    return r;
  }
  if (!isDead) {
    if (static_cast<double>(curHits) <= static_cast<double>(maxHits) * kRescueAliveFraction) {
      r.message = name + " was found alive, and in pretty bad shape, so we healed them and brought them back!";
      r.broughtBack = true;
    } else {
      r.message = name + " was found, but was alive and well!";
    }
    return r;
  }
  if (isRock) {
    r.message = "The only thing they saw were feet sticking out of rock.";
    return r;
  }
  r.message = name + " was found at " + std::to_string(x) + "," + std::to_string(y) + "," +
              std::to_string(level) + " and brought to the morgue!";
  r.broughtBack = true;
  return r;
}

// A body is a dead character. Alive and Stoned are not bodies.
// UNKNOWN: which death states count as a body; the rest are treated as dead.
bool rescueIsBody(const Character& c) {
  return c.deathState != DeathState::Alive && c.deathState != DeathState::Stoned;
}

// Rescue.CarryHome: DungeonLevel = 0 and Area = -1. X and Y are left alone.
void rescueCarryHome(Character& c) {
  c.location.dungeonLevel = static_cast<decltype(c.location.dungeonLevel)>(0);
  c.location.area = static_cast<decltype(c.location.area)>(-1);
}

// The square at explicit coordinates on a floor (accSquareHere with the coordinates given).
SquareState* rescueSquareAt(Session& s, i32 level, i32 x, i32 y) {
  if (level < 1) return nullptr;
  i32 width = kRescueDefaultWidth;
  for (const FloorDef& d : s.data.floors) {
    if (d.levelNum == level) {
      width = d.width;
      break;
    }
  }
  for (FloorState& f : s.game.floors) {
    if (f.dungeonLevel != level) continue;
    const i32 idx = (y - 1) * width + (x - 1);
    if (idx < 0 || static_cast<size_t>(idx) >= f.squares.size()) return nullptr;
    return &f.squares[static_cast<size_t>(idx)];
  }
  return nullptr;
}

// The body's square: RESCUE doubles the chance every day.
bool rescueEasyToFind(Session& s, const Character& c) {
  const SquareState* sq = rescueSquareAt(s, static_cast<i32>(c.location.dungeonLevel),
                                         static_cast<i32>(c.location.x), static_cast<i32>(c.location.y));
  if (sq == nullptr) return false;
  // NEED: the Floor RESCUE bit on SquareState. Until it is named, no square doubles the odds.
  return false;
}

// NEED: morgue rescuer fee. Not in evidence, so it is unpriced.
// UNKNOWN: whether the fee is charged before or after the search. Here it is charged first.
i64 rescueFeeFor(const Session&) {
  return kRescueFeeUnpriced;
}

// Takes the fee from the paying member's gold on hand. Returns false with the reason in reply.
bool rescueChargeFee(Session& s, Str& reply) {
  const i64 fee = rescueFeeFor(s);
  if (fee < 0) {
    reply = "The morgue's rescuers are not priced, so no one is sent out.";
    return false;
  }
  Character* payer = accMember(s, kRescuePayerMember);
  if (payer == nullptr) {
    reply = "No party member can pay the morgue.";
    return false;
  }
  Currency& gold = accGoldOnHand(*payer);
  if (gold < fee) {
    reply = "You cannot afford the morgue's rescuers.";
    return false;
  }
  gold -= fee;
  return true;
}

// ---- pending raise: state lives in featureState as "raise.<member>.<field>" ----

Str raiseKey(i32 member, const char* field) {
  return "raise." + std::to_string(member) + "." + field;
}

// A raise is pending when its kind is recorded. Initiate writes the kind only for a chance above 0.
bool raiseRecorded(const Session& s, i32 member) {
  return s.featureState.count(raiseKey(member, "kind")) > 0;
}

i32 raiseGetInt(const Session& s, i32 member, const char* field, i32 fallback) {
  auto it = s.featureState.find(raiseKey(member, field));
  if (it == s.featureState.end()) return fallback;
  i32 out = fallback;
  if (!rescueParseInt(it->second, out)) return fallback;
  return out;
}

void raiseSetInt(Session& s, i32 member, const char* field, i32 value) {
  s.featureState[raiseKey(member, field)] = std::to_string(value);
}

void raiseClear(Session& s, i32 member) {
  s.featureState.erase(raiseKey(member, "chance"));
  s.featureState.erase(raiseKey(member, "kind"));
  s.featureState.erase(raiseKey(member, "by"));
}

Str raiseBy(const Session& s, i32 member) {
  auto it = s.featureState.find(raiseKey(member, "by"));
  return it == s.featureState.end() ? Str() : it->second;
}

// Raising.cs: the age a raise costs, by the state of the body.
i32 raiseAgeCost(DeathState st) {
  if (st == DeathState::Rock) return kRaiseAgeRock;
  if (st == DeathState::Stoned) return kRaiseAgeStoned;
  return kRaiseAgeOrdinary;
}

// NEED: race lifespan field on RaceDef. Until it is named, a failed non-rock raise never complicates.
// Reference: (ageDays / 365) / (lifespan / 2) * 100.
i32 rescueComplicationPercent(const Session&, const Character&) {
  return 0;
}

}  // namespace

// PendingRaise.Initiate (sub0A7C, MAINMOD2.BAS:5974-5998). The body is moved to the raiser's
// square, level and area. A chance above 0 writes the raise on it.
// Declared for the spell and morgue modules; NEED: declare in features.h and call from those modules.
void rescueInitiatePendingRaise(Session& s, i32 bodyMember, i32 raiserMember, i32 chance, i32 kind) {
  Character* body = accMember(s, bodyMember);
  const Character* raiser = accMember(s, raiserMember);
  if (body == nullptr || raiser == nullptr) return;
  body->location.x = static_cast<decltype(body->location.x)>(raiser->location.x);
  body->location.y = static_cast<decltype(body->location.y)>(raiser->location.y);
  body->location.dungeonLevel = static_cast<decltype(body->location.dungeonLevel)>(raiser->location.dungeonLevel);
  body->location.area = static_cast<decltype(body->location.area)>(raiser->location.area);
  if (chance > 0) {
    raiseSetInt(s, bodyMember, "chance", chance);
    raiseSetInt(s, bodyMember, "kind", kind);
    s.featureState[raiseKey(bodyMember, "by")] = raiser->name;
  }
}

namespace {

// Raising.Resolve plus PendingRaise.Prepare. Runs when the dead character is loaded.
// Order: the age cost is always charged; then the success roll; a failure may complicate.
// UNKNOWN: the success test is taken as Int(Rnd * 100) < chance, the community reading of the box.
// UNKNOWN: hit points after a successful raise. One is assumed.
bool rescueResolvePendingRaise(Ctx& ctx, Session& s, i32 member, Str& reply) {
  Character* c = accMember(s, member);
  if (c == nullptr) {
    reply = "There is no party member " + std::to_string(member) + ".";
    return false;
  }
  if (!raiseRecorded(s, member)) {
    reply = c->name + " has no raise pending.";
    return false;
  }

  // PendingRaise.Prepare: rock zeroes the chance before the offer, so the roll can only fail.
  i32 chance = raiseGetInt(s, member, "chance", 0);
  if (c->deathState == DeathState::Rock) chance = 0;
  const Str by = raiseBy(s, member);
  ctx.say("raise.offer", "Chance of success is " + std::to_string(chance) + "%.", {chance}, {by});

  c->ageDays += static_cast<decltype(c->ageDays)>(raiseAgeCost(c->deathState));
  const bool success = static_cast<i32>(std::floor(ctx.rng.nextDouble() * 100.0)) < chance;
  raiseClear(s, member);

  if (success) {
    c->deathState = DeathState::Alive;
    c->vitals.curHits = static_cast<decltype(c->vitals.curHits)>(1);
    reply = c->name + " is raised!";
    return true;
  }

  // Failure. Rock always complicates; otherwise the complication roll decides (MAINMOD2.BAS:1651).
  const bool complicate =
      c->deathState == DeathState::Rock ||
      static_cast<i32>(std::floor(ctx.rng.nextDouble() * 100.0)) < rescueComplicationPercent(s, *c);
  if (!complicate) {
    reply = "Raise Failed!";
    return true;
  }
  c->ageDays += static_cast<decltype(c->ageDays)>(kComplicationAgeDays);
  // MAINMOD2.BAS:1664: MaxHits = Int(MaxHits / 1.2).
  c->vitals.maxHits = static_cast<decltype(c->vitals.maxHits)>(
      std::floor(static_cast<double>(c->vitals.maxHits) / kComplicationHitsDivisor));
  reply = "Raise Failed! " + c->name + " comes back fifteen years older with fewer hit points.";
  return true;
}

}  // namespace

// Dungeon: "rescue <member>". The dead member lies until found, then is carried to town.
bool rescueVerbDungeon(Session& s, const Vec<Str>& w, Str& reply) {
  i32 member = 0;
  if (w.size() != 2 || !rescueParseInt(w[1], member)) {
    reply = "Usage: rescue <member>  - lie where you fell until found and carried to town";
    return false;
  }
  Character* c = accMember(s, member);
  if (c == nullptr) {
    reply = "There is no party member " + std::to_string(member) + ".";
    return false;
  }
  if (!rescueIsBody(*c)) {
    reply = c->name + " is not dead.";
    return false;
  }
  if (c->location.dungeonLevel < 1) {
    reply = c->name + " is already in town.";
    return false;
  }

  const bool easy = rescueEasyToFind(s, *c);
  Ctx ctx = s.ctx();
  const i32 days = rescueDaysUntilFound(ctx, static_cast<i32>(c->location.dungeonLevel),
                                        static_cast<i32>(c->location.area), easy);
  c->ageDays += static_cast<decltype(c->ageDays)>(days);
  rescueCarryHome(*c);

  reply = "You have been found after " + std::to_string(days) + " day(s)!";
  return true;
}

// Town. "rescue <member>" takes up a pending raise on the body.
// "rescue <member> <x> <y> <level>" pays the morgue to send rescuers to the guessed spot.
bool rescueVerbTown(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() == 2) {
    i32 member = 0;
    if (!rescueParseInt(w[1], member)) {
      reply = "Usage: rescue <member>  - in town, resolve the raise pending on the body";
      return false;
    }
    Ctx ctx = s.ctx();
    return rescueResolvePendingRaise(ctx, s, member, reply);
  }

  i32 member = 0;
  i32 x = 0;
  i32 y = 0;
  i32 level = 0;
  if (w.size() != 5 || !rescueParseInt(w[1], member) || !rescueParseInt(w[2], x) ||
      !rescueParseInt(w[3], y) || !rescueParseInt(w[4], level)) {
    reply = "Usage: rescue <member>  - resolve a pending raise, or rescue <member> <x> <y> <level>  - send the morgue's rescuers";
    return false;
  }
  Character* c = accMember(s, member);
  if (c == nullptr) {
    reply = "There is no party member " + std::to_string(member) + ".";
    return false;
  }
  if (c->location.dungeonLevel < 1) {
    reply = c->name + " is already in town.";
    return false;
  }

  // The fee is checked before any search. Nothing changes if it cannot be paid.
  if (!rescueChargeFee(s, reply)) return false;

  const i32 bodyX = static_cast<i32>(c->location.x);
  const i32 bodyY = static_cast<i32>(c->location.y);
  const i32 bodyLevel = static_cast<i32>(c->location.dungeonLevel);
  const bool found = rescueCloseEnough(x, y, level, bodyX, bodyY, bodyLevel);
  const bool dead = rescueIsBody(*c);
  const bool rock = c->deathState == DeathState::Rock;
  const RescueReport r = rescueReport(c->name, found, dead, rock,
                                      static_cast<i32>(c->vitals.curHits),
                                      static_cast<i32>(c->vitals.maxHits),
                                      bodyX, bodyY, bodyLevel);
  reply = r.message;
  if (r.broughtBack) {
    if (!dead) {
      // UNKNOWN: the heal for a living find is taken as full hit points.
      c->vitals.curHits = c->vitals.maxHits;
    } else {
      // Brought to the morgue: carried home like a rescued body.
      rescueCarryHome(*c);
    }
  }
  return true;
}

void register_rescue() {
  registerFeatureCommand({"rescue", Screen::Dungeon, &rescueVerbDungeon,
                          "rescue <member>  - lie where you fell until found and carried to town"});
  registerFeatureCommand({"rescue", Screen::Town, &rescueVerbTown,
                          "rescue <member> <x> <y> <level>  - pay the morgue to send rescuers"});
}

}  // namespace mordor
