// Command registry: a feature adds player commands without editing the screen handlers. No I/O; portable to Unreal.
// A feature file (src/feat_<id>.cpp) defines handlers and a function `void register_<id>()` that registers them.
#pragma once
#include "mordor/game.h"

namespace mordor {

// words[0] is the verb as typed; the handler returns true when the command was carried out (a refusal that explains itself in `reply` is still true).
using FeatureFn = bool (*)(Session& s, const Vec<Str>& words, Str& reply);

struct FeatureCommand {
  const char* verb;    // lower case
  Screen screen;       // Screen::Town, Screen::Dungeon or Screen::Combat; register once per screen
  FeatureFn fn;
  const char* help;    // one line shown by `help`
};

void registerFeatureCommand(const FeatureCommand& c);
void registerAllFeatures();   // feature_registry.cpp: calls every feature's register function (idempotent)

// True when a registered feature owns the verb on this screen; `ok` is the handler's result.
bool runFeatureCommand(Session& s, Screen screen, const Str& command, Str& reply, bool& ok);
Str featureHelp(Screen screen);

}  // namespace mordor
