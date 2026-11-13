#include "mordor/features.h"
#include "mordor/access.h"

#include <cmath>
#include <string>
#include "mordor/shared_helpers.h"

namespace mordor {
namespace {

// Pending person offer, kept in s.featureState until it is answered.
const char* const kSeerKindKey = "seer.kind";
const char* const kSeerNameKey = "seer.name";
const char* const kSeerLevelKey = "seer.level";

const char* const kSeerInTheCity = "Do you mock my abilities?!\nThey are here in the city!";
const char* const kSeerNotEnoughGold = "Not enough gold!";
const char* const kSeerNoSuchMonster = "I am unable to locate a creature called '";
const char* const kSeerNoSuchItem = "I am unable to locate an item by the name of '";
const char* const kSeerSeveralMonsters = "The name you provided is shared by more than one creature.";
const char* const kSeerSeveralItems = "The name you provided is shared by more than one item.";
const char* const kSeerCannotDetect = "I am unable to detect the creature in the depths below. ";

// R63, FRMSEEK.BAS:336. Person search: fixed fee of the depth they are at, no random element.
i64 seerPersonFee(i32 level) {
  const double lv = static_cast<double>(level);
  return static_cast<i64>(132567.52 * std::log(lv + 1.0) * lv);
}

// FRMSEEK.BAS:340-352. Roll against three times the level; a near miss gives the level only.
Str seerLocateCharacterReply(Ctx& ctx, const Str& name, i32 level, i32 x, i32 y, bool dead) {
  const i32 need = level * 3;
  const i32 roll = ctx.rng.below(100) + 1;
  if (roll < need) {
    if (roll + 10 >= need) {
      return "I can detect them somewhere on level " + std::to_string(level) + ", but I've lost connection.";
    }
    return "I cannot locate \"" + name + "\".  Another try may work, though.";
  }
  Str said = "I have detected \"" + name + "\" at: " + std::to_string(x) + "," + std::to_string(y) + "," +
             std::to_string(level) + ".";
  if (dead) said += "\nI also feel that they are among the dead.";
  return said;
}

// FRMSEEK.BAS:66-86, 224-233. Case-insensitive match against the start of the name.
char seerLower(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + ('a' - 'A')) : c; }

bool seerStartsWith(const Str& name, const Str& typed) {
  if (name.size() < typed.size()) return false;
  for (size_t i = 0; i < typed.size(); ++i) {
    if (seerLower(name[i]) != seerLower(typed[i])) return false;
  }
  return true;
}

bool seerSameText(const Str& a, const Str& b) { return a.size() == b.size() && seerStartsWith(a, b); }

enum class SeerMatchKind : i32 { None = 0, Several, One };

struct SeerMatchResult {
  SeerMatchKind kind;
  i32 index;
};

// An exact match wins outright. Two or more partial matches: he refuses to guess.
SeerMatchResult seerMatch(const Str& typed, const Vec<Str>& names) {
  if (typed.empty()) return {SeerMatchKind::None, -1};
  i32 count = 0;
  i32 last = -1;
  for (size_t i = 0; i < names.size(); ++i) {
    if (!seerStartsWith(names[i], typed)) continue;
    last = static_cast<i32>(i);
    ++count;
    if (seerSameText(names[i], typed)) return {SeerMatchKind::One, last};
  }
  if (count == 0) return {SeerMatchKind::None, -1};
  if (count == 1) return {SeerMatchKind::One, last};
  return {SeerMatchKind::Several, -1};
}

Vec<Str> seerMonsterNames(const Session& s) {
  Vec<Str> out;
  for (const MonsterDef& d : s.data.monsters) out.push_back(d.name);
  return out;
}

Vec<Str> seerItemNames(const Session& s) {
  Vec<Str> out;
  for (const ItemDef& d : s.data.items) out.push_back(d.name);
  return out;
}

Vec<Str> seerMemberNames(const Session& s) {
  Vec<Str> out;
  for (i32 i = 1; i <= accPartySize(s); ++i) out.push_back(accMember(const_cast<Session&>(s), i)->name);
  return out;
}


// Pending offer bookkeeping.
bool seerHasPending(const Session& s) {
  auto it = s.featureState.find(kSeerKindKey);
  return it != s.featureState.end() && !it->second.empty();
}

Str seerGet(const Session& s, const char* key) {
  auto it = s.featureState.find(key);
  return it == s.featureState.end() ? Str() : it->second;
}

void seerClearPending(Session& s) {
  s.featureState.erase(kSeerKindKey);
  s.featureState.erase(kSeerNameKey);
  s.featureState.erase(kSeerLevelKey);
}

void seerSetPendingPerson(Session& s, const Str& name, i32 level) {
  s.featureState[kSeerKindKey] = "person";
  s.featureState[kSeerNameKey] = name;
  s.featureState[kSeerLevelKey] = std::to_string(level);
}

// Gold is taken from the party's gold on hand, in roster order.
// UNKNOWN: which member pays. Bank gold is not counted.
bool seerTakeGold(Session& s, Currency amount) {
  Currency total = 0;
  for (i32 i = 1; i <= accPartySize(s); ++i) total += accGoldOnHand(*accMember(s, i));
  if (total < amount) return false;
  Currency left = amount;
  for (i32 i = 1; i <= accPartySize(s) && left > 0; ++i) {
    Currency& gold = accGoldOnHand(*accMember(s, i));
    const Currency take = gold < left ? gold : left;
    gold -= take;
    left -= take;
  }
  return true;
}

// NEED: the level a creature or item first appears on (monster and item table fields).
// Until it is linked, the fee and the level gate cannot be sized, so the search stops here.
bool seerThingLevel(const Session& s, bool item, i32 index, i32& level) {
  (void)s;
  (void)item;
  (void)index;
  (void)level;
  return false;
}

// "seer creature|item <name>": match the name, then the level is needed to go further.
bool seerSearchThing(Session& s, bool item, const Str& typed, Str& reply) {
  const Vec<Str> names = item ? seerItemNames(s) : seerMonsterNames(s);
  const SeerMatchResult m = seerMatch(typed, names);
  if (m.kind == SeerMatchKind::None) {
    reply = item ? kSeerNoSuchItem + typed + "'." : kSeerNoSuchMonster + typed + "'.";
    return true;
  }
  if (m.kind == SeerMatchKind::Several) {
    reply = item ? kSeerSeveralItems : kSeerSeveralMonsters;
    return true;
  }
  i32 level = 0;
  if (!seerThingLevel(s, item, m.index, level)) {
    reply = kSeerCannotDetect;
    return true;
  }
  reply = kSeerCannotDetect;  // unreachable until seerThingLevel is linked
  return true;
}

// "seer person <name>": the fee is fixed by the depth they are at; the offer waits for yes or no.
bool seerSearchPerson(Session& s, const Str& typed, Str& reply) {
  const Vec<Str> names = seerMemberNames(s);
  const SeerMatchResult m = seerMatch(typed, names);
  if (m.kind == SeerMatchKind::None) {
    reply = "I am unable to locate a character called '" + typed + "'.";  // UNKNOWN: text not in reference
    return true;
  }
  if (m.kind == SeerMatchKind::Several) {
    reply = "The name you provided is shared by more than one character.";  // UNKNOWN: text not in reference
    return true;
  }
  Character* c = accMember(s, m.index + 1);
  const i32 level = c->location.dungeonLevel;
  if (level < 1) {
    reply = kSeerInTheCity;
    return true;
  }
  const i64 fee = seerPersonFee(level);
  seerSetPendingPerson(s, c->name, level);
  reply = "I can feel their presense.  I will need " + storeGoldText(fee) + " to attempt to locate " + c->name +
          ".\nIs this acceptable?";
  return true;
}

// "seer yes" / "seer no": a yes pays the fee, then runs the search.
bool seerAnswer(Session& s, bool yes, Str& reply) {
  if (!seerHasPending(s)) {
    reply = "Nothing is awaiting your answer.";
    return true;
  }
  if (!yes) {
    seerClearPending(s);
    reply = "Cancelled.";  // UNKNOWN: reference shows no text for declining
    return true;
  }
  const Str name = seerGet(s, kSeerNameKey);
  i64 levelWide = 0;
  accParseInt(seerGet(s, kSeerLevelKey), levelWide);
  const i32 level = static_cast<i32>(levelWide);
  seerClearPending(s);

  const i32 member = accMemberByName(s, name);
  if (member == 0) {
    reply = "Nothing is awaiting your answer.";
    return true;
  }
  if (!seerTakeGold(s, seerPersonFee(level))) {
    reply = kSeerNotEnoughGold;
    return true;
  }
  Ctx ctx = s.ctx();
  Character* c = accMember(s, member);
  reply = seerLocateCharacterReply(ctx, c->name, level, c->location.x, c->location.y,
                                   c->deathState != DeathState::Alive);
  return true;
}

}  // namespace

bool seerCommand(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() < 2) {
    reply = "seer creature <name>  - pay the Seer to search for a creature\n"
            "seer item <name>  - pay the Seer to search for an item\n"
            "seer person <name>  - pay the Seer to locate a person\n"
            "seer yes  /  seer no  - answer the Seer's offer";
    return true;
  }
  const Str sub = accLower(w[1]);
  if (sub == "yes" || sub == "no") return seerAnswer(s, sub == "yes", reply);
  if (sub == "creature" || sub == "item") {
    if (w.size() < 3) {
      reply = "Name what you seek.";
      return true;
    }
    return seerSearchThing(s, sub == "item", seerJoinWords(w, 2), reply);
  }
  if (sub == "person") {
    if (w.size() < 3) {
      reply = "Name what you seek.";
      return true;
    }
    return seerSearchPerson(s, seerJoinWords(w, 2), reply);
  }
  reply = "The Seer does not understand that request.";
  return true;
}

void register_seer() {
  registerFeatureCommand({"seer", Screen::Town, &seerCommand,
                          "seer <creature|item|person> <name> | seer yes | seer no  - the Seer searches for a fee"});
}

}  // namespace mordor
