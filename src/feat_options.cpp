#include "mordor/features.h"

#include <cctype>
#include <string>

#include "mordor/state_common.h"
#include "mordor/shared_helpers.h"

namespace mordor {
namespace {

// GameOptions.cs: the twelve checkboxes in form order, then the volume and brightness fields.
// Names are the reference captions' INI keys. Matching is case-insensitive, as Ini is.
const char* const kGameBoolNames[] = {"No3d", "NoMusic", "NoFX", "NoStretch", "SoundCanvas", "NoDing",
                                      "NoLoadAutomap", "NoToolbar", "NoPartyFX", "NoHelpSound",
                                      "NoAutoSave", "NoBlinkCursor"};
constexpr i32 kGameBoolCount = 12;
const char* const kGameIntNames[] = {"MusicVolume", "FXVolume", "Brightness"};
constexpr i32 kGameIntCount = 3;
const char* const kLastBackupName = "LastBackup";  // read-only here; written by Backup and Restore
// UNKNOWN: valid range for the volumes and Brightness; the reference text is truncated. 0..100 assumed.
constexpr i32 kLevelMax = 100;
// UNKNOWN: FXVolume default; the reference uses WaveVolume.Default, whose value is not supplied.
constexpr i32 kFxVolumeIndex = 1;

// CharacterOptions.cs: names and the defaults its constructor sets (only AutoChangeLevel starts on).
const char* const kCharNames[] = {"AutoChangeLevel", "Hide", "PreloadCharacter", "NoPerceptionMessages"};
constexpr i32 kCharCount = 4;
const bool kCharDefaults[] = {true, false, false, false};


bool sameName(const Str& word, const char* name) { return lowerAscii(word) == lowerAscii(Str(name)); }

i32 findIndex(const char* const* names, i32 count, const Str& word) {
  for (i32 i = 0; i < count; ++i)
    if (sameName(word, names[static_cast<size_t>(i)])) return i;
  return -1;
}


bool parseBool(const Str& t, bool& out) {
  Str l = lowerAscii(t);
  if (l == "on" || l == "true" || l == "yes" || l == "1") { out = true; return true; }
  if (l == "off" || l == "false" || l == "no" || l == "0") { out = false; return true; }
  return false;
}

bool parseLevel(const Str& t, i32& out) {
  if (!isDigits(t) || t.size() > 3) return false;
  i32 v = 0;
  for (char c : t) v = v * 10 + (c - '0');
  if (v > kLevelMax) return false;
  out = v;
  return true;
}

// Parses a party slot number 1..kPartyMaxSize; returns -1 when invalid.
i32 parseMember(const Str& t) {
  if (!isDigits(t) || t.size() > 2) return -1;
  i32 v = 0;
  for (char c : t) v = v * 10 + (c - '0');
  if (v < 1 || v > kPartyMaxSize) return -1;
  return v;
}

Str gameKey(const char* name) { return Str("option.") + name; }

Str charKey(i32 member, const char* name) {
  return Str("option.char") + static_cast<char>('0' + member) + "." + name;
}

Str storedOr(const Session& s, const Str& key, const Str& dflt) {
  return featureStateOr(s, key, dflt);
}

Str gameBoolText(const Session& s, i32 i) {
  return storedOr(s, gameKey(kGameBoolNames[static_cast<size_t>(i)]), "0") == "1" ? "on" : "off";
}

Str gameIntText(const Session& s, i32 i) {
  const char* dflt = (i == kFxVolumeIndex) ? "(default)" : "0";
  return storedOr(s, gameKey(kGameIntNames[static_cast<size_t>(i)]), dflt);
}

Str charBoolText(const Session& s, i32 member, i32 idx) {
  const char* dflt = kCharDefaults[static_cast<size_t>(idx)] ? "1" : "0";
  return storedOr(s, charKey(member, kCharNames[static_cast<size_t>(idx)]), dflt) == "1" ? "on" : "off";
}

Str usageText() {
  return "usage: options | options <name> [on|off|0-100] | options <member 1-4> <name> [on|off]";
}

Str listAll(const Session& s) {
  Str out = "Game options:";
  for (i32 i = 0; i < kGameBoolCount; ++i)
    out += "\n  " + Str(kGameBoolNames[static_cast<size_t>(i)]) + ": " + gameBoolText(s, i);
  for (i32 i = 0; i < kGameIntCount; ++i)
    out += "\n  " + Str(kGameIntNames[static_cast<size_t>(i)]) + ": " + gameIntText(s, i);
  Str last = storedOr(s, gameKey(kLastBackupName), "");
  out += "\n  Last Backup was on " + (last.empty() ? Str("(none)") : last);
  out += "\nCharacter options (by party member):";
  for (i32 m = 1; m <= kPartyMaxSize; ++m)
    for (i32 c = 0; c < kCharCount; ++c)
      out += "\n  member " + Str(1, static_cast<char>('0' + m)) + " " +
             kCharNames[static_cast<size_t>(c)] + ": " + charBoolText(s, m, c);
  return out;
}

Str gameCommand(Session& s, const Vec<Str>& w) {
  if (w.size() > 3) return usageText();
  const Str& name = w[1];

  if (sameName(name, kLastBackupName)) {
    if (w.size() == 2) return "Last Backup was on " + storedOr(s, gameKey(kLastBackupName), "(none)");
    return "LastBackup is set by Backup and Restore, not by options.";
  }

  i32 b = findIndex(kGameBoolNames, kGameBoolCount, name);
  i32 n = findIndex(kGameIntNames, kGameIntCount, name);
  if (b < 0 && n < 0) return "Unknown option: " + name + ".\n" + usageText();

  if (w.size() == 2) {
    if (b >= 0) return Str(kGameBoolNames[static_cast<size_t>(b)]) + ": " + gameBoolText(s, b);
    return Str(kGameIntNames[static_cast<size_t>(n)]) + ": " + gameIntText(s, n);
  }

  const Str& value = w[2];
  if (b >= 0) {
    bool on = false;
    if (!parseBool(value, on)) return "Use on or off for " + name + ".";
    s.featureState[gameKey(kGameBoolNames[static_cast<size_t>(b)])] = on ? "1" : "0";
    return Str(kGameBoolNames[static_cast<size_t>(b)]) + " set to " + (on ? "on" : "off") + ".";
  }
  i32 v = 0;
  if (!parseLevel(value, v)) return "Use a number from 0 to 100 for " + name + ".";
  s.featureState[gameKey(kGameIntNames[static_cast<size_t>(n)])] = std::to_string(v);
  return Str(kGameIntNames[static_cast<size_t>(n)]) + " set to " + std::to_string(v) + ".";
}

Str charCommand(Session& s, const Vec<Str>& w) {
  if (w.size() < 3 || w.size() > 4) return usageText();
  i32 member = parseMember(w[1]);
  if (member < 0) return "Party member must be 1 to 4.";
  i32 idx = findIndex(kCharNames, kCharCount, w[2]);
  if (idx < 0) return "Unknown character option: " + w[2] + ".";

  if (w.size() == 3)
    return "member " + w[1] + " " + kCharNames[static_cast<size_t>(idx)] + ": " + charBoolText(s, member, idx);

  bool on = false;
  if (!parseBool(w[3], on)) return "Use on or off for " + w[2] + ".";
  s.featureState[charKey(member, kCharNames[static_cast<size_t>(idx)])] = on ? "1" : "0";
  return "member " + w[1] + " " + kCharNames[static_cast<size_t>(idx)] + " set to " + (on ? "on" : "off") + ".";
}

// UNKNOWN: assumed w[0] is the verb word, so w[1] is the first argument.
bool cmdOptions(Session& s, const Vec<Str>& w, Str& reply) {
  if (w.size() <= 1) {
    reply = listAll(s);
    return true;
  }
  if (isDigits(w[1])) {
    reply = charCommand(s, w);
    return true;
  }
  reply = gameCommand(s, w);
  return true;
}

}  // namespace

void register_options() {
  registerFeatureCommand({"options", Screen::Town, &cmdOptions,
                          "options [name [value]] | options <member> <name> [on|off]  - list or change options"});
  registerFeatureCommand({"options", Screen::Dungeon, &cmdOptions,
                          "options [name [value]] | options <member> <name> [on|off]  - list or change options"});
}

}  // namespace mordor
