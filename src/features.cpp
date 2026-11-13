#include "mordor/features.h"

namespace mordor {

namespace {
Vec<FeatureCommand>& table() {
  static Vec<FeatureCommand> t;
  return t;
}
Str lowerAscii(const Str& w) {
  Str o = w;
  for (char& c : o) {
    if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
  }
  return o;
}
Vec<Str> splitWords(const Str& text) {
  Vec<Str> out;
  Str cur;
  for (char c : text) {
    if (c == ' ' || c == '\t') {
      if (!cur.empty()) { out.push_back(cur); cur.clear(); }
    } else {
      cur.push_back(c);
    }
  }
  if (!cur.empty()) out.push_back(cur);
  return out;
}
}  // namespace

void registerFeatureCommand(const FeatureCommand& c) { table().push_back(c); }

bool runFeatureCommand(Session& s, Screen screen, const Str& command, Str& reply, bool& ok) {
  registerAllFeatures();
  const Vec<Str> w = splitWords(command);
  if (w.empty()) return false;
  const Str verb = lowerAscii(w[0]);
  for (const FeatureCommand& c : table()) {
    if (c.screen == screen && verb == c.verb) {
      ok = c.fn(s, w, reply);
      return true;
    }
  }
  return false;
}

Str featureHelp(Screen screen) {
  registerAllFeatures();
  Str out;
  for (const FeatureCommand& c : table()) {
    if (c.screen == screen) { out += "  "; out += c.help; out += "\n"; }
  }
  return out;
}

}  // namespace mordor
