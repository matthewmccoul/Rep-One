#include "mordor/features.h"

#include <algorithm>
#include <cstddef>
#include "mordor/shared_helpers.h"

namespace mordor {
namespace {

// Librarian.cs: knowledge grades. Monsters 1-4; items use 4 = all known, otherwise almost everything.
constexpr int kLibVeryLittle = 1;
constexpr int kLibAGoodDealMore = 2;
constexpr int kLibAlmostEverything = 3;
constexpr int kLibEverything = 4;

// Librarian.cs word lists (MODULE1.BAS:454-533).
constexpr const char* kSizes[] = {"Very Small", "Small", "Normal", "Big", "Very Big", "Huge"};
constexpr const char* kAlignments[] = {"Good", "Neutral", "Evil"};
constexpr const char* kMonsterProperties[] = {"See Invisible", "Invisible", "Resistant to All Magic",
                                              "Charm Resistant", "Weapon Resistance", "Complete Weapon Resistance"};
constexpr const char* kMonsterAttacks[] = {"Poison", "Disease", "Paralyze", "Breathe Fire", "Breathe Cold",
                                           "Acid", "Electrocute", "Drain", "Stone", "Age", "Critical Hit",
                                           "Backstab", "Destroy Item", "Steal"};
constexpr const char* kSpellSchools[] = {"Fire", "Cold", "Electrical", "Mind", "Damage", "Element", "Kill",
                                         "Charm", "Bind", "Heal", "Movement", "Banish", "Dispell", "Resistant",
                                         "Visual", "Magical", "Location", "Protection"};
constexpr const char* kResistances[] = {"Fire", "Cold", "Electrical", "Mind", "Disease", "Poison", "Magic",
                                        "Stone", "Paralysis", "Drain", "Acid"};
constexpr const char* kItemAbilities[] = {"Levitate", "Invisible", "Protect", "See Invisible",
                                          "Critical Hit", "Backstab"};

// UNKNOWN: assumed layout of the party's Library records (not in evidence).
//   "library.<kind>.<name>"           -> grade ("1".."4"); kind is "monster" or "item"
//   "library.<kind>.<name>.<field>"   -> fact value (integers, or bitmasks for lists)
constexpr const char* kRecordPrefix = "library.";
constexpr const char* kKindMonster = "monster";
constexpr const char* kKindItem = "item";

struct LibEntry {
  Str kind;
  Str name;
  int grade;
};

Str upperCase(const Str& t) {
  Str r = t;
  for (char& c : r) {
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
  }
  return r;
}

int parseInt(const Str& t, int fallback) {
  if (t.empty()) return fallback;
  int v = 0;
  for (char c : t) {
    if (c < '0' || c > '9') return fallback;
    v = v * 10 + (c - '0');
    if (v > 1000000) return fallback;
  }
  return v;
}


Str factStr(const Session& s, const Str& kind, const Str& name, const char* field) {
  auto it = s.featureState.find(Str(kRecordPrefix) + kind + "." + name + "." + field);
  return it == s.featureState.end() ? Str() : it->second;
}

template <std::size_t N>
Str nameAt(const char* const (&names)[N], int i) {
  if (i >= 0 && i < static_cast<int>(N)) return Str(names[i]);
  return Str();
}

template <std::size_t N>
Str listedBits(int mask, const char* const (&names)[N]) {
  Str r;
  for (std::size_t b = 0; b < N; ++b) {
    if ((mask >> b) & 1) {
      if (!r.empty()) r += ", ";
      r += names[b];
    }
  }
  return r;
}

// Every record in the Library, sorted by name. UNKNOWN: Windows word sort (Librarian LibrarySearch)
// is approximated by case-insensitive ASCII order.
Vec<LibEntry> entries(const Session& s) {
  Vec<LibEntry> out;
  const std::size_t prefixLen = Str(kRecordPrefix).size();
  for (const auto& kv : s.featureState) {
    const Str& key = kv.first;
    if (key.compare(0, prefixLen, kRecordPrefix) != 0) continue;
    Str rest = key.substr(prefixLen);
    std::size_t dot = rest.find('.');
    if (dot == Str::npos) continue;
    Str kind = rest.substr(0, dot);
    Str name = rest.substr(dot + 1);
    if (name.find('.') != Str::npos) continue;  // a fact key, not a record
    if (kind != kKindMonster && kind != kKindItem) continue;
    int grade = parseInt(kv.second, 0);
    if (grade < kLibVeryLittle) continue;  // grade 0 = not in the Library
    out.push_back(LibEntry{kind, name, grade});
  }
  std::sort(out.begin(), out.end(), [](const LibEntry& a, const LibEntry& b) {
    Str ua = upperCase(a.name);
    Str ub = upperCase(b.name);
    if (ua != ub) return ua < ub;
    return a.kind < b.kind;
  });
  return out;
}

Str monsterPage(const Session& s, const LibEntry& e) {
  const Str& name = e.name;
  Str out = name + "\n";
  if (e.grade >= kLibVeryLittle) {
    int size = parseInt(factStr(s, kKindMonster, name, "size"), -1);
    Str word = nameAt(kSizes, size);
    out += "Size: " + (word.empty() ? Str("oddly") : word) + "\n";  // Librarian.Size(): "oddly" when out of range
  }
  if (e.grade >= kLibAGoodDealMore) {
    // UNKNOWN: class line of the grade-2 page (T-F) is not in evidence; placeholder reads the fact "class".
    Str cls = factStr(s, kKindMonster, name, "class");
    out += "Class: " + (cls.empty() ? Str("Unknown") : cls) + "\n";
  }
  if (e.grade >= kLibAlmostEverything) {
    // UNKNOWN: grade-3 statistics layout and its "nobody knows" wording (T-F) are not in evidence.
    out += "Statistics: " + factStr(s, kKindMonster, name, "stats") + "\n";
    out += "What it can do is not known.\n";
  }
  if (e.grade >= kLibEverything) {
    int align = parseInt(factStr(s, kKindMonster, name, "alignment"), -1);
    Str alignWord = nameAt(kAlignments, align);
    if (!alignWord.empty()) out += "Alignment: " + alignWord + "\n";
    int props = parseInt(factStr(s, kKindMonster, name, "properties"), 0);
    int attacks = parseInt(factStr(s, kKindMonster, name, "attacks"), 0);
    int spells = parseInt(factStr(s, kKindMonster, name, "spells"), 0);
    int resist = parseInt(factStr(s, kKindMonster, name, "resistances"), 0);
    Str p = listedBits(props, kMonsterProperties);
    Str a = listedBits(attacks, kMonsterAttacks);
    Str sp = listedBits(spells, kSpellSchools);
    Str r = listedBits(resist, kResistances);
    out += "Properties: " + (p.empty() ? Str("None") : p) + "\n";  // UNKNOWN: "None" wording
    out += "Attacks: " + (a.empty() ? Str("None") : a) + "\n";
    out += "Spells: " + (sp.empty() ? Str("None") : sp) + "\n";
    out += "Resistances: " + (r.empty() ? Str("None") : r) + "\n";
  }
  return out;
}

Str itemPage(const Session& s, const LibEntry& e) {
  Str out = e.name + "\n";
  if (e.grade >= kLibEverything) {
    int abilities = parseInt(factStr(s, kKindItem, e.name, "abilities"), 0);
    Str a = listedBits(abilities, kItemAbilities);
    out += "Abilities: " + (a.empty() ? Str("None") : a) + "\n";
    // UNKNOWN: damage modifier, curse, spell, effects and guild levels (T-G) are not in evidence.
  } else {
    // UNKNOWN: wording and content of the below-4 item page (T-G) are not in evidence.
    out += "Not everything is known.\n";
  }
  return out;
}

Str pageFor(const Session& s, const LibEntry& e) {
  return e.kind == kKindMonster ? monsterPage(s, e) : itemPage(s, e);
}

bool findEntry(const Vec<LibEntry>& all, const Str& kind, const Str& name, LibEntry& out) {
  Str want = upperCase(name);
  for (const LibEntry& e : all) {
    if (e.kind == kind && upperCase(e.name) == want) {
      out = e;
      return true;
    }
  }
  return false;
}

const Str kUsage = "Usage: library | library search <words> | library monster <name> | library item <name>";

// UNKNOWN: assumes w[0] is the verb "library" and w[1] the subcommand.
bool cmdLibrary(Session& s, const Vec<Str>& w, Str& reply) {
  Vec<LibEntry> all = entries(s);
  if (w.size() <= 1) {
    if (all.empty()) {
      reply = "The library is empty.";  // UNKNOWN: wording
      return true;
    }
    Str out;
    for (const LibEntry& e : all) {
      out += e.name + "\n";
    }
    reply = out;
    return true;
  }

  const Str& sub = w[1];
  if (sub == "search") {
    Str typed = upperCase(joinFrom(w, 2));
    Vec<std::size_t> hits;
    for (std::size_t i = 0; i < all.size(); ++i) {
      Str nm = upperCase(all[i].name);
      if (nm.compare(0, typed.size(), typed) == 0) hits.push_back(i);
    }
    if (hits.empty()) {
      reply = "No match was found.";
    } else if (hits.size() == 1) {
      reply = pageFor(s, all[hits[0]]);  // the one match is selected, so its page shows
    } else {
      reply = "ambiguous!";
    }
    return true;
  }

  if (sub == "monster" || sub == "item") {
    Str name = joinFrom(w, 2);
    if (name.empty()) {
      reply = kUsage;
      return true;
    }
    LibEntry e;
    const Str kind = (sub == "monster") ? Str(kKindMonster) : Str(kKindItem);
    if (!findEntry(all, kind, name, e)) {
      reply = "There is no entry for " + name + " in the library.";  // UNKNOWN: wording
      return true;
    }
    reply = pageFor(s, e);
    return true;
  }

  reply = kUsage;
  return true;
}

}  // namespace

void register_library() {
  registerFeatureCommand({"library", Screen::Town, &cmdLibrary,
                          "library [search <words> | monster <name> | item <name>]  - what the party has learned"});
}

}  // namespace mordor
