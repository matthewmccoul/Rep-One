#include "mordor/features.h"
#include "mordor/access.h"

#include <string>
#include "mordor/shared_helpers.h"

namespace mordor {
namespace {

// R86 / Hall of Records (HallRecords.cs). Twelve records, in the file's order:
// 0 Strength, 1 Intelligence, 2 Wisdom, 3 Constitution, 4 Charisma, 5 Dexterity,
// 6 DeadliestCreature, 7 MostExperience, 8 Wealthiest, 9 Fighting, 10 Magic, 11 Thieving.
// State lives in s.featureState under "hall.<k>.<field>":
//   holder  name of the record holder ("" = nobody yet)
//   key     what is compared: monster power for the creature record, else the value
//   value   what is shown: monster id for the creature record, 1/10000 gold for Wealthiest
//   day     day serial the record was set (0 = never)

constexpr i32 kHallCount = 12;
constexpr i32 kHallConstitution = 3;
constexpr i32 kHallDeadliest = 6;
constexpr i32 kHallMostExperience = 7;
constexpr i32 kHallWealthiest = 8;
constexpr i32 kHallFirstSkill = 9;
constexpr i32 kHallLastSkill = 11;

// Records a party member is offered on arrival in town (HallRecords.Check, MODULE11.BAS sub1043).
// Skills 9-11 need fn1BF0; the creature record 6 is offered by hallRecordKill, not on arrival.
constexpr i32 kArrivalRecords[] = {0, 1, 2, 3, 4, 5, 7, 8};

struct HallRow {
  const char* heading;  // HallRecords.Heading (FRMHALL.BAS sub32F6)
  const char* logName;  // HallRecords.LogName (MODULE11.BAS:473-500)
};

HallRow hallRow(i32 k) {
  switch (k) {
    case 0: return {"Strongest", "Strongest Character"};
    case 1: return {"Smartest", "most Intelligent Character"};
    case 2: return {"Wisest", "Wisest Character"};
    case 3: return {"Healthiest", "Healthiest Character"};
    case 4: return {"Most Attractive", "most Attractive Character"};
    case 5: return {"Quickest", "Quickest Character"};
    case 6: return {"Deadliest Creature Defeated", "most powerful creature defeated"};
    case 7: return {"Most Experienced Explorer", "most experience"};
    case 8: return {"Wealthiest Explorer", "wealthiest character"};
    case 9: return {"Master of Fighting", "highest fighting ability"};
    case 10: return {"Master of Magic", "highest spell knowledge & power level"};
    default: return {"Master of Thieving", "highest thieving ability"};
  }
}

Str hallKey(i32 k, const char* field) {
  return "hall." + std::to_string(k) + "." + field;
}

// Plain decimal parse; never throws. Unreadable text reads as 0.
i64 hallParseI(const Str& t) {
  bool neg = false;
  size_t i = 0;
  if (i < t.size() && t[i] == '-') {
    neg = true;
    ++i;
  }
  i64 v = 0;
  for (; i < t.size(); ++i) {
    char c = t[i];
    if (c < '0' || c > '9') break;
    v = v * 10 + (c - '0');
  }
  return neg ? -v : v;
}

i64 hallGetI(const Session& s, i32 k, const char* field) {
  return hallParseI(glGet(s, hallKey(k, field)));
}

// VB CStr of a Currency held in 1/10000 gold: no trailing zeros, "0.25" style.
Str hallCurrencyText(i64 v) {
  bool neg = v < 0;
  i64 a = neg ? -v : v;
  i64 whole = a / 10000;
  i64 frac = a % 10000;
  Str out = neg ? "-" : "";
  out += std::to_string(whole);
  if (frac != 0) {
    Str digits;
    for (int d = 0; d < 4; ++d) {
      digits += static_cast<char>('0' + frac / 1000);
      frac = (frac % 1000) * 10;
    }
    while (!digits.empty() && digits.back() == '0') digits.pop_back();
    out += "." + digits;
  }
  return out;
}

Str hallValueText(i32 k, i64 v) {
  if (k == kHallDeadliest) return "Monster id " + std::to_string(v);  // UNKNOWN: creature record value text (FRMHALL)
  if (k == kHallWealthiest) return hallCurrencyText(v) + " gold";     // UNKNOWN: exact gold wording
  if (k >= kHallFirstSkill && k <= kHallLastSkill) return std::to_string(v) + "%";  // UNKNOWN: skill percentage wording
  return std::to_string(v);
}

// HallRecords.Describe with FRMHALL sub32F6 layout.
// UNKNOWN: date text; the day serial is shown as a number because no date formatter is available.
Str hallDescribe(const Session& s, i32 k) {
  HallRow row = hallRow(k);
  Str holder = glGet(s, hallKey(k, "holder"));
  i64 day = hallGetI(s, k, "day");
  Str who = day != 0 ? "Set on day " + std::to_string(day) + " by " : Str("Set by Original RecordHolder ");
  Str out = Str(row.heading) + "\n";
  out += who + "\"" + holder + "\"\n";
  out += hallValueText(k, hallGetI(s, k, "value"));
  return out;
}

// hall: show the Hall of Records.
bool cmdHall(Session& s, const Vec<Str>& w, Str& reply) {
  (void)w;
  Str out = "Hall of Records";
  for (i32 k = 0; k < kHallCount; ++k) out += "\n" + hallDescribe(s, k);
  reply = out;
  return true;
}

}  // namespace

void register_hall() {
  registerFeatureCommand({"hall", Screen::Town, &cmdHall, "hall  - the Hall of Records: the twelve town-wide bests"});
}

}  // namespace mordor
